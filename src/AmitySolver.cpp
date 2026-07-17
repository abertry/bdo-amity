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

    bool lessThanOrNearlyEqual(double left, double right) {
        const double scale = std::max({1.0, std::abs(left), std::abs(right)});
        return left <= right + 1e-10 * scale;
    }

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

    void validateKnowledge(const Knowledge& knowledge) {
        if (knowledge.favorMin > knowledge.favorMax) {
            throw std::invalid_argument("knowledge favorMin must not exceed favorMax");
        }
        for (const ComboEffect& effect : knowledge.comboEffects) {
            if (effect.delay < 1) {
                throw std::invalid_argument("combo delay must be at least one turn");
            }
            if (effect.duration < 1) {
                throw std::invalid_argument("combo duration must be at least one turn");
            }
        }
    }

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
        state.currentFavor = 0;
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
        state.currentFavor += favorGain;
        state.accumulatedFavor += state.currentFavor;
        return state;
    }

    struct TurnModifiers {
        int npcFavor = 0;
        int npcInterest = 0;
        int topicFavor = 0;
        int topicInterest = 0;
    };

    TurnModifiers modifiersForTurn(
        int position,
        const std::vector<std::size_t>& order,
        const std::vector<Knowledge>& available
    ) {
        TurnModifiers modifiers;
        for (int sourcePosition = 0; sourcePosition < position; ++sourcePosition) {
            for (const ComboEffect& effect : available[order[sourcePosition]].comboEffects) {
                const int firstAffectedTurn = sourcePosition + effect.delay;
                if (position < firstAffectedTurn || position >= firstAffectedTurn + effect.duration) {
                    continue;
                }
                switch (effect.type) {
                case ComboEffectType::NpcFavor: modifiers.npcFavor += effect.amount; break;
                case ComboEffectType::NpcInterest: modifiers.npcInterest += effect.amount; break;
                case ComboEffectType::TopicFavor: modifiers.topicFavor += effect.amount; break;
                case ComboEffectType::TopicInterest: modifiers.topicInterest += effect.amount; break;
                }
            }
        }
        return modifiers;
    }

    double sparkChance(const Knowledge& knowledge, const Npc& npc, const TurnModifiers& modifiers) {
        const int npcInterest = std::max(0, npc.interest + modifiers.npcInterest);
        const int topicInterest = std::max(0, knowledge.interest + modifiers.topicInterest);
        if (npcInterest <= 0) {
            return 1.0;
        }
        return std::clamp(static_cast<double>(topicInterest) / npcInterest, 0.0, 1.0);
    }

    StateDistribution advance(
        const StateDistribution& distribution,
        const Knowledge& knowledge,
        const Npc& npc,
        const TurnModifiers& modifiers = {}
    ) {
        StateDistribution next;
        const double spark = sparkChance(knowledge, npc, modifiers);

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
                const int effectiveNpcFavor = npc.favor + modifiers.npcFavor;
                const int gain = std::max(1, favor + modifiers.topicFavor - effectiveNpcFavor);
                next[afterSpark(state, gain)] += outcomeProbability;
            }
        }
        return next;
    }

    struct Evaluation {
        double satisfactionProbability = 0.0;
        double expectedFavor = 0.0;
        double expectedMaximumFavor = 0.0;
    };

    Evaluation evaluate(const StateDistribution& distribution, const Goal& goal) {
        Evaluation result;
        for (const auto& entry : distribution) {
            if (satisfies(entry.first, goal)) {
                result.satisfactionProbability += entry.second;
            }
            result.expectedFavor += entry.second * entry.first.accumulatedFavor;
            result.expectedMaximumFavor += entry.second * entry.first.currentFavor;
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
            averageGain += std::max(1, favor - npc.favor);
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
                validateKnowledge(knowledge);
                hasComboEffects_ = hasComboEffects_ || !knowledge.comboEffects.empty();
                expectedContributions_.push_back(expectedContribution(knowledge, npc));
            }
        }

        SolverResult run() {
            StateDistribution initial{{ConversationState{}, 1.0}};
            seedIncumbent(initial);
            search(0, initial);
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

            // Seed only affects search order and initial incumbent, never correctness.
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
                const int position = static_cast<int>(best_.order.size());
                distribution = advance(distribution, available_[index], npc_,
                    modifiersForTurn(position, seedOrder_, available_));
                best_.order.push_back(available_[index]);
                seedOrder_.push_back(index);
            }
            const Evaluation seeded = evaluate(distribution, goal_);
            best_.satisfactionProbability = seeded.satisfactionProbability;
            best_.expectedAccumulatedFavor = seeded.expectedFavor;
            best_.expectedMaximumFavor = seeded.expectedMaximumFavor;
            best_.exploredOrders = 1;
        }

        void search(int position, const StateDistribution& distribution) {
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
                    best_.expectedMaximumFavor = candidate.expectedMaximumFavor;
                }
                return;
            }

            // Combo-free optimistic bound: preserve current expected MFL through
            // every future turn (ignore resets), then add each topic's expected
            // gain high-first. Actual failures can only lower MFL/AFL.
            if (!hasComboEffects_ && best_.satisfactionProbability >= 1.0 - EPSILON) {
                std::vector<double> optimisticGains;
                for (std::size_t i = 0; i < available_.size(); ++i) {
                    if ((usedMask_ & (std::uint64_t{1} << i)) == 0) {
                        optimisticGains.push_back(expectedContributions_[i]);
                    }
                }
                std::sort(optimisticGains.begin(), optimisticGains.end(), std::greater<double>());
                const int needed = slots_ - position;
                double expectedMaximumFavor = 0.0;
                double upperExpectedFavor = 0.0;
                for (const auto& entry : distribution) {
                    expectedMaximumFavor += entry.second * entry.first.currentFavor;
                    upperExpectedFavor += entry.second * entry.first.accumulatedFavor;
                }
                for (int turn = 0; turn < needed; ++turn) {
                    expectedMaximumFavor += optimisticGains[turn];
                    upperExpectedFavor += expectedMaximumFavor;
                }
                if (lessThanOrNearlyEqual(upperExpectedFavor, best_.expectedAccumulatedFavor)) {
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
                const StateDistribution next = advance(distribution, available_[index], npc_,
                    modifiersForTurn(position, order_, available_));
                search(position + 1, next);
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
        std::vector<std::size_t> seedOrder_;
        bool hasComboEffects_ = false;
        SolverResult best_;
    };
}

double AmitySolver::sparkChance(const Knowledge& knowledge, const Npc& npc) {
    if (npc.interest <= 0) {
        return 1.0;
    }

    return std::clamp(static_cast<double>(knowledge.interest) / npc.interest, 0.0, 1.0);
}

double AmitySolver::expectedFavorGain(const Knowledge& knowledge, const Npc& npc) {
    double total = 0.0;
    for (int favor = knowledge.favorMin; favor <= knowledge.favorMax; ++favor) {
        total += std::max(1, favor - npc.favor);
    }
    return total / (knowledge.favorMax - knowledge.favorMin + 1);
}

SolverResult AmitySolver::evaluateOrder(
    const std::vector<Knowledge>& order,
    const Goal& goal,
    const Npc& npc
) {
    if (npc.slots < 0) {
        throw std::invalid_argument("npc slots must not be negative");
    }
    for (const Knowledge& knowledge : order) {
        validateKnowledge(knowledge);
    }
    StateDistribution distribution{{ConversationState{}, 1.0}};
    std::vector<std::size_t> indices;
    const int turns = std::min<int>(npc.slots, order.size());
    indices.reserve(turns);
    for (int position = 0; position < turns; ++position) {
        distribution = advance(distribution, order[position], npc,
            modifiersForTurn(position, indices, order));
        indices.push_back(static_cast<std::size_t>(position));
    }
    const Evaluation evaluation = evaluate(distribution, goal);
    SolverResult result;
    result.order.assign(order.begin(), order.begin() + turns);
    result.satisfactionProbability = evaluation.satisfactionProbability;
    result.expectedAccumulatedFavor = evaluation.expectedFavor;
    result.expectedMaximumFavor = evaluation.expectedMaximumFavor;
    result.exploredOrders = 1;
    return result;
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
