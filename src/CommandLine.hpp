#pragma once

#include "Goal.hpp"
#include "Npc.hpp"

#include <iosfwd>
#include <string>

struct CommandLineOptions {
    Npc npc{0, 0, 0};
    Goal goal{GoalType::FreeTalk, 0};
    std::string category;
    bool showHelp = false;
    bool listCategories = false;
};

CommandLineOptions parseCommandLine(int argc, const char* const argv[]);
void printUsage(std::ostream& output, const char* executableName);
