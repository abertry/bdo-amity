#include "AmitySolver.hpp"
#include "CommandLine.hpp"
#include "KnowledgeBase.hpp"

#include <exception>
#include <iomanip>
#include <iostream>

int main(int argc, const char* argv[]) {
    try {
        const CommandLineOptions options = parseCommandLine(argc, argv);
        if (options.showHelp) {
            printUsage(std::cout, argv[0]);
            return 0;
        }
        if (options.listCategories) {
            for (const auto& category : KnowledgeBase::categories()) {
                std::cout << category << '\n';
            }
            return 0;
        }

        const auto available = KnowledgeBase::selectCategory(options.category);
        if (available.empty()) {
            std::cerr << "Unknown or empty knowledge category: " << options.category << '\n';
            std::cerr << "Use --list-categories to view valid categories.\n";
            return 2;
        }

        Npc npc = options.npc;
        npc.name = options.category;
        const SolverResult result = AmitySolver::solveExact(available, options.goal, npc);
        AmitySolver::printOrder(result.order, options.goal, npc);
        std::cout << "\nGoal probability: " << std::fixed << std::setprecision(2)
            << result.satisfactionProbability * 100.0 << "%\n"
            << "Expected accumulated favor: " << result.expectedAccumulatedFavor << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << "\n\n";
        printUsage(std::cerr, argc > 0 ? argv[0] : "amity_solver");
        return 1;
    }
}