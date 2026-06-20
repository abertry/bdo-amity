#include "AmitySolver.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>

namespace {
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
}

double AmitySolver::sparkChance(const Knowledge& knowledge, const Npc& npc) {
    if (npc.interest <= 0.0) {
        return 1.0;
    }

    return std::min(knowledge.interest / npc.interest, 1.0);
}

double AmitySolver::expectedFavorGain(const Knowledge& knowledge, const Npc& npc) {
    return std::max(0.0, knowledge.avgFavor() - npc.favor);
}

double AmitySolver::scoreKnowledge(
    const Knowledge& knowledge,
    const Goal& goal,
    const Npc& npc,
    int position
) {
    const double p = sparkChance(knowledge, npc);
    const double fail = 1.0 - p;
    const double favor = expectedFavorGain(knowledge, npc);

    switch (goal.type) {
    case GoalType::Spark:
        return p * 1000.0 + favor;

    case GoalType::ConsecutiveSpark:
        // For consecutive success, early reliability is dominant.
        return p * 10000.0 + favor - position * 0.01;

    case GoalType::FailSpark:
        return fail * 1000.0 + favor * 0.1;

    case GoalType::ConsecutiveFail:
        // For consecutive fail, early failure chance is dominant.
        return fail * 10000.0 + favor * 0.1 - position * 0.01;

    case GoalType::AccumulatedFavor:
        // Accumulated favor benefits from reliable early favor.
        return p * favor * 100.0 + p * 10.0;

    case GoalType::MinimumFavor:
        return p * favor * 100.0 + favor;

    case GoalType::FreeTalk:
        return p * favor * 100.0 + favor;
    }

    return 0.0;
}

std::vector<Knowledge> AmitySolver::solve(
    std::vector<Knowledge> available,
    const Goal& goal,
    const Npc& npc
) {
    std::sort(
        available.begin(),
        available.end(),
        [&](const Knowledge& a, const Knowledge& b) {
            const double pa = sparkChance(a, npc);
            const double pb = sparkChance(b, npc);
            const double fa = expectedFavorGain(a, npc);
            const double fb = expectedFavorGain(b, npc);

            switch (goal.type) {
            case GoalType::Spark:
            case GoalType::ConsecutiveSpark:
                if (pa != pb) return pa > pb;
                return fa > fb;

            case GoalType::FailSpark:
            case GoalType::ConsecutiveFail:
                if (pa != pb) return pa < pb;
                return fa > fb;

            case GoalType::AccumulatedFavor:
                // Reliable expected favor first.
                if (pa * fa != pb * fb) return pa * fa > pb * fb;
                if (pa != pb) return pa > pb;
                return fa > fb;

            case GoalType::MinimumFavor:
            case GoalType::FreeTalk:
                // Maximum expected value.
                if (pa * fa != pb * fb) return pa * fa > pb * fb;
                return fa > fb;
            }

            return a.name < b.name;
        }
    );

    if (available.size() > static_cast<size_t>(npc.slots)) {
        available.resize(npc.slots);
    }

    return available;
}

void AmitySolver::printOrder(
    const std::vector<Knowledge>& order,
    const Goal& goal,
    const Npc& npc
) {
    std::cout << "NPC: " << npc.name
        << " | Interest: " << npc.interest
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
