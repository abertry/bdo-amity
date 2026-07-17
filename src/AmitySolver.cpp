#include "AmitySolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <unordered_map>

namespace {
    constexpr double EPSILON = 1e-12;

    struct ConversationState {
        int currentFavor = 0;
        int accumulatedFavor = 0;
        int sparkCount = 0;
        int failCount = 0;
        int currentSparkChain = 0;
        int currentFailChain = 0;
        int longestSparkChain = 0;
        int longestFailChain = 0;

        bool operator==(const ConversationState& other) const {
            return currentFavor == other.currentFavor
                && accumulatedFavor == other.accumulatedFavor
                && sparkCount == other.sparkCount
                && failCount == other.failCount
                && currentSparkChain == other.currentSparkChain
                && currentFailChain == other.currentFailChain
                && longestSparkChain == other.longestSparkChain
                && longestFailChain == other.longestFailChain;
        }
    };

    struct StateHash {
        std::size_t operator()(const ConversationState& state) const {
            std::size_t seed = 0;
            const auto combine = [&seed](int value) {
                seed ^= std::hash<int>{}(value) + 0x9e3779b9U
                    + (seed << 6U) + (seed >> 2U);
            };
            combine(state.currentFavor);
            combine(state.accumulatedFavor);
            combine(state.sparkCount);
            combine(state.failCount);
            combine(state.currentSparkChain);
            combine(state.currentFailChain);
            combine(state.longestSparkChain);
            combine(state.longestFailChain);
            return seed;
        }
    };

    using StateDistribution = std::unordered_map<ConversationState, double, StateHash>;

    const char* goalName(GoalType type) {
        switch (type) {
        case GoalType::Spark: return "Spark";
        case GoalType::FailSpark: return "FailSpark";
        case GoalType::ConsecutiveSpark: return "ConsecutiveSpark";
        case GoalType::ConsecutiveFail: return "ConsecutiveFail";
        case GoalType::AccumulatedFavor: return "AccumulatedFavor";
        case GoalType::MinimumFavor: return "MaxFavor";
        case GoalType::FreeTalk: return "FreeTalk";
        }

        return "Unknown";
    }

    bool satisfies(const ConversationState& state, const Goal& goal) {
        if (goal.target <= 0 || goal.type == GoalType::FreeTalk) {
            return true;
        }

        switch (goal.type) {
        case GoalType::Spark: return state.sparkCount >= goal.target;
        case GoalType::FailSpark: return state.failCount >= goal.target;
        case GoalType::ConsecutiveSpark: return state.longestSparkChain >= goal.target;
        case GoalType::ConsecutiveFail: return state.longestFailChain >= goal.target;
        case GoalType::AccumulatedFavor: return state.accumulatedFavor >= goal.target;
        case GoalType::MinimumFavor: return state.currentFavor >= goal.target;
        case GoalType::FreeTalk: return true;
        }
        return false;
    }

    ConversationState afterFail(ConversationState state) {
        ++state.failCount;
        state.currentSparkChain = 0;
        ++state.currentFailChain;
        state.longestFailChain = std::max(state.longestFailChain, state.currentFailChain);
        return state;
    }

    ConversationState afterSpark(ConversationState state, int favorGain) {
        ++state.sparkCount;
        ++state.currentSparkChain;
        state.currentFailChain = 0;
        state.longestSparkChain = std::max(state.longestSparkChain, state.currentSparkChain);
        state.currentFavor = std::max(state.currentFavor, favorGain);
        state.accumulatedFavor += favorGain;
        return state;
    }

    StateDistribution advance(
        const StateDistribution& distribution,
        const Knowledge& knowledge,
        const Npc& npc
    ) {
        StateDistribution next;
        const double spark = AmitySolver::sparkChance(knowledge, npc);

        for (const auto& entry : distribution) {
            const ConversationState& state = entry.first;
            const double stateProbability = entry.second;

            if (spark < 1.0) {
                next[afterFail(state)] += stateProbability * (1.0 - spark);
            }

            if (spark <= 0.0) {
                continue;
            }

            const int favorOutcomes = knowledge.favorMax - knowledge.favorMin + 1;
            const double outcomeProbability = stateProbability * spark / favorOutcomes;
            for (int favor = knowledge.favorMin; favor <= knowledge.favorMax; ++favor) {
                const int gain = std::max(0, favor - npc.favor);
                next[afterSpark(state, gain)] += outcomeProbability;
            }
        }
        return next;
    }

    struct Evaluation {
        double satisfactionProbability = 0.0;
        double expectedFavor = 0.0;
    };

    Evaluation evaluate(const StateDistribution& distribution, const Goal& goal) {
        Evaluation result;
        for (const auto& entry : distribution) {
            if (satisfies(entry.first, goal)) {
                result.satisfactionProbability += entry.second;
            }
            result.expectedFavor += entry.second * entry.first.accumulatedFavor;
        }
        return result;
    }

    bool isBetter(const Evaluation& candidate, const SolverResult& best) {
        if (candidate.satisfactionProbability > best.satisfactionProbability + EPSILON) {
            return true;
        }
        return std::abs(candidate.satisfactionProbability - best.satisfactionProbability) <= EPSILON
            && candidate.expectedFavor > best.expectedAccumulatedFavor + EPSILON;
    }

    double expectedContribution(const Knowledge& knowledge, const Npc& npc) {
        double averageGain = 0.0;
        for (int favor = knowledge.favorMin; favor <= knowledge.favorMax; ++favor) {
            averageGain += std::max(0, favor - npc.favor);
        }
        averageGain /= knowledge.favorMax - knowledge.favorMin + 1;
        return AmitySolver::sparkChance(knowledge, npc) * averageGain;
    }

    class ExactSearch {
    public:
        ExactSearch(const std::vector<Knowledge>& available, const Goal& goal, const Npc& npc)
            : available_(available), goal_(goal), npc_(npc), slots_(std::min<int>(npc.slots, available.size())) {
            expectedContributions_.reserve(available.size());
            for (const auto& knowledge : available) {
                if (knowledge.favorMin > knowledge.favorMax) {
                    throw std::invalid_argument("knowledge favorMin must not exceed favorMax");
                }
                expectedContributions_.push_back(expectedContribution(knowledge, npc));
            }
        }

        SolverResult run() {
            StateDistribution initial{{ConversationState{}, 1.0}};
            seedIncumbent(initial);
            search(0, initial, 0.0);
            return best_;
        }

    private:
        void seedIncumbent(const StateDistribution& initial) {
            std::vector<std::size_t> indices(available_.size());
            std::iota(indices.begin(), indices.end(), 0);
            std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
                return expectedContributions_[a] > expectedContributions_[b];
            });
            indices.resize(slots_);

            // Expected favor is additive and independent of position. Arrange the
            // maximum-reward selection to give chain goals their strongest chance.
            if (goal_.type == GoalType::ConsecutiveSpark) {
                std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
                    return AmitySolver::sparkChance(available_[a], npc_)
                        > AmitySolver::sparkChance(available_[b], npc_);
                });
            } else if (goal_.type == GoalType::ConsecutiveFail) {
                std::stable_sort(indices.begin(), indices.end(), [&](std::size_t a, std::size_t b) {
                    return AmitySolver::sparkChance(available_[a], npc_)
                        < AmitySolver::sparkChance(available_[b], npc_);
                });
            }

            StateDistribution distribution = initial;
            for (std::size_t index : indices) {
                distribution = advance(distribution, available_[index], npc_);
                best_.order.push_back(available_[index]);
            }
            const Evaluation seeded = evaluate(distribution, goal_);
            best_.satisfactionProbability = seeded.satisfactionProbability;
            best_.expectedAccumulatedFavor = seeded.expectedFavor;
            best_.exploredOrders = 1;
        }

        void search(int position, const StateDistribution& distribution, double expectedFavor) {
            if (position == slots_) {
                ++best_.exploredOrders;
                const Evaluation candidate = evaluate(distribution, goal_);
                if (best_.order.empty() || isBetter(candidate, best_)) {
                    best_.order.clear();
                    for (std::size_t index : order_) {
                        best_.order.push_back(available_[index]);
                    }
                    best_.satisfactionProbability = candidate.satisfactionProbability;
                    best_.expectedAccumulatedFavor = candidate.expectedFavor;
                }
                return;
            }

            // Once probability 1 is attained, expected favor is the only possible
            // improvement. This admissible bound discards selections that cannot win.
            if (!best_.order.empty() && best_.satisfactionProbability >= 1.0 - EPSILON) {
                std::vector<double> remaining;
                for (std::size_t i = 0; i < available_.size(); ++i) {
                    if ((usedMask_ & (std::uint64_t{1} << i)) == 0) {
                        remaining.push_back(expectedContributions_[i]);
                    }
                }
                std::sort(remaining.begin(), remaining.end(), std::greater<double>());
                const int needed = slots_ - position;
                const double upperExpected = expectedFavor + std::accumulate(
                    remaining.begin(), remaining.begin() + needed, 0.0
                );
                if (upperExpected <= best_.expectedAccumulatedFavor + EPSILON) {
                    ++best_.prunedBranches;
                    return;
                }
            }

            std::vector<std::size_t> candidates;
            for (std::size_t i = 0; i < available_.size(); ++i) {
                if ((usedMask_ & (std::uint64_t{1} << i)) == 0) {
                    candidates.push_back(i);
                }
            }
            std::stable_sort(candidates.begin(), candidates.end(), [&](std::size_t a, std::size_t b) {
                return expectedContributions_[a] > expectedContributions_[b];
            });

            for (std::size_t index : candidates) {
                usedMask_ |= std::uint64_t{1} << index;
                order_.push_back(index);
                const StateDistribution next = advance(distribution, available_[index], npc_);
                search(position + 1, next, expectedFavor + expectedContributions_[index]);
                order_.pop_back();
                usedMask_ &= ~(std::uint64_t{1} << index);
            }
        }

        const std::vector<Knowledge>& available_;
        const Goal& goal_;
        const Npc& npc_;
        int slots_;
        std::vector<double> expectedContributions_;
        std::uint64_t usedMask_ = 0;
        std::vector<std::size_t> order_;
        SolverResult best_;
    };
}

double AmitySolver::sparkChance(const Knowledge& knowledge, const Npc& npc) {
    if (npc.interest <= 0.0) {
        return 1.0;
    }

    return std::clamp(knowledge.interest / npc.interest, 0.0, 1.0);
}

double AmitySolver::expectedFavorGain(const Knowledge& knowledge, const Npc& npc) {
    return std::max(0.0, knowledge.avgFavor() - npc.favor);
}

SolverResult AmitySolver::solveExact(
    const std::vector<Knowledge>& available,
    const Goal& goal,
    const Npc& npc
) {
    if (npc.slots < 0) {
        throw std::invalid_argument("npc slots must not be negative");
    }
    if (available.size() > 63) {
        throw std::invalid_argument("exact solver supports at most 63 knowledge entries");
    }
    if (npc.slots == 0 || available.empty()) {
        const ConversationState initial;
        SolverResult result;
        result.satisfactionProbability = satisfies(initial, goal) ? 1.0 : 0.0;
        result.exploredOrders = 1;
        return result;
    }
    return ExactSearch(available, goal, npc).run();
}

std::vector<Knowledge> AmitySolver::solve(
    std::vector<Knowledge> available,
    const Goal& goal,
    const Npc& npc
) {
    return solveExact(available, goal, npc).order;
}

void AmitySolver::printOrder(
    const std::vector<Knowledge>& order,
    const Goal& goal,
    const Npc& npc
) {
    std::cout << "NPC Interest: " << npc.interest
        << " | Favor: " << npc.favor
        << " | Slots: " << npc.slots << "\n";

    std::cout << "Goal: " << goalName(goal.type)
        << " | Target: " << goal.target << "\n\n";

    int index = 1;

    for (const auto& knowledge : order) {
        const double p = sparkChance(knowledge, npc);
        const double fail = 1.0 - p;
        const double favor = expectedFavorGain(knowledge, npc);

        std::cout
            << index++ << ". "
            << knowledge.name
            << " | Interest: " << knowledge.interest
            << " | Favor: " << knowledge.favorMin << "~" << knowledge.favorMax
            << " | Spark: " << std::fixed << std::setprecision(1) << p * 100.0 << "%"
            << " | Fail: " << fail * 100.0 << "%"
            << " | Expected gain: " << favor
            << "\n";
    }
}
