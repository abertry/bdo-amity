#pragma once

#include "Goal.hpp"
#include "Knowledge.hpp"
#include "Npc.hpp"

#include <vector>

struct KnowledgeScore {
    Knowledge knowledge;
    double sparkChance;
    double expectedFavorGain;
    double score;
};

class AmitySolver {
public:
    static double sparkChance(const Knowledge& knowledge, const Npc& npc);

    static double expectedFavorGain(const Knowledge& knowledge, const Npc& npc);

    static std::vector<Knowledge> solve(
        std::vector<Knowledge> available,
        const Goal& goal,
        const Npc& npc
    );

    static void printOrder(
        const std::vector<Knowledge>& order,
        const Goal& goal,
        const Npc& npc
    );

private:
    static double scoreKnowledge(
        const Knowledge& knowledge,
        const Goal& goal,
        const Npc& npc,
        int position
    );
};
