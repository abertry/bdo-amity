#pragma once

#include "Goal.hpp"
#include "Knowledge.hpp"
#include "Npc.hpp"

#include <cstddef>
#include <vector>

struct SolverResult {
    std::vector<Knowledge> order;
    double satisfactionProbability = 0.0;
    double expectedAccumulatedFavor = 0.0;
    std::size_t exploredOrders = 0;
    std::size_t prunedBranches = 0;
};

class AmitySolver {
public:
    static double sparkChance(const Knowledge& knowledge, const Npc& npc);

    static double expectedFavorGain(const Knowledge& knowledge, const Npc& npc);

    // Finds the exact best fixed order. Goal satisfaction probability is the
    // primary objective; expected accumulated favor breaks ties.
    static SolverResult solveExact(
        const std::vector<Knowledge>& available,
        const Goal& goal,
        const Npc& npc
    );

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

};
