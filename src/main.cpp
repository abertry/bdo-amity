#include "AmitySolver.hpp"
#include "KnowledgeBase.hpp"
#include "Npc.hpp"
#include "Goal.hpp"

#include <vector>

int main() {
    Npc npc{
        "Tranan Underfoe",
        30,
        15,
        8
    };

    Goal goal{
        GoalType::ConsecutiveSpark,
        3
    };

    std::vector<std::string> availableNames = {
        "Islin Bartali",
        "Crio",
        "David Finto",
        "Alustin",
        "Igor Bartali",
        "Artemio Fiazza",
        "Claire Arryn",
        "Abelin",
        "Mariano",
        "Egrin",
        "Hans",
        "Dalishain",
        "Houtman",
        "Momo",
    };

    auto available = KnowledgeBase::select(availableNames);

    auto order = AmitySolver::solve(
        available,
        goal,
        npc
    );

    AmitySolver::printOrder(order, goal, npc);

    return 0;
}
