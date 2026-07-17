#include "CommandLine.hpp"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace {
std::string normalize(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    std::replace(value.begin(), value.end(), '_', '-');
    return value;
}

GoalType parseGoalType(const std::string& value) {
    static const std::unordered_map<std::string, GoalType> goals{
        {"spark", GoalType::Spark},
        {"fail-spark", GoalType::FailSpark},
        {"consecutive-spark", GoalType::ConsecutiveSpark},
        {"consecutive-fail", GoalType::ConsecutiveFail},
        {"accumulated-favor", GoalType::AccumulatedFavor},
        {"minimum-favor", GoalType::MinimumFavor},
        {"free-talk", GoalType::FreeTalk}
    };
    const auto found = goals.find(normalize(value));
    if (found == goals.end()) {
        throw std::invalid_argument("unknown goal: " + value);
    }
    return found->second;
}

std::string requireValue(int& index, int argc, const char* const argv[]) {
    if (++index >= argc) {
        throw std::invalid_argument(std::string("missing value for ") + argv[index - 1]);
    }
    return argv[index];
}

int parseInteger(const std::string& value, const std::string& flag) {
    std::size_t consumed = 0;
    int result = 0;
    try { result = std::stoi(value, &consumed); }
    catch (const std::exception&) { throw std::invalid_argument(flag + " requires an integer"); }
    if (consumed != value.size()) {
        throw std::invalid_argument(flag + " requires an integer");
    }
    return result;
}

}

CommandLineOptions parseCommandLine(int argc, const char* const argv[]) {
    CommandLineOptions options;
    bool hasInterest = false;
    bool hasFavor = false;
    bool hasSlots = false;
    bool hasCategory = false;
    bool hasGoal = false;
    bool hasTarget = false;

    for (int index = 1; index < argc; ++index) {
        const std::string flag = argv[index];
        if (flag == "--help" || flag == "-h") options.showHelp = true;
        else if (flag == "--list-categories") options.listCategories = true;
        else if (flag == "--interest") {
            options.npc.interest = parseInteger(requireValue(index, argc, argv), flag);
            hasInterest = true;
        } else if (flag == "--favor") {
            options.npc.favor = parseInteger(requireValue(index, argc, argv), flag);
            hasFavor = true;
        } else if (flag == "--slots") {
            options.npc.slots = parseInteger(requireValue(index, argc, argv), flag);
            hasSlots = true;
        } else if (flag == "--category") {
            options.category = requireValue(index, argc, argv);
            hasCategory = true;
        } else if (flag == "--goal") {
            options.goal.type = parseGoalType(requireValue(index, argc, argv));
            hasGoal = true;
        } else if (flag == "--target") {
            options.goal.target = parseInteger(requireValue(index, argc, argv), flag);
            hasTarget = true;
        } else {
            throw std::invalid_argument("unknown option: " + flag);
        }
    }

    if (options.showHelp || options.listCategories) return options;
    if (!hasInterest || !hasFavor || !hasSlots || !hasCategory || !hasGoal || !hasTarget) {
        throw std::invalid_argument(
            "required options: --interest --favor --slots --category --goal --target"
        );
    }
    if (options.npc.interest < 0 || options.npc.favor < 0 || options.npc.slots <= 0
        || options.goal.target < 0) {
        throw std::invalid_argument("interest, favor, and target must be non-negative; slots must be positive");
    }
    return options;
}

void printUsage(std::ostream& output, const char* executableName) {
    output << "Usage: " << executableName << " [options]\n\n"
        << "Required options:\n"
        << "  --interest NUMBER       NPC interest\n"
        << "  --favor NUMBER          NPC favor\n"
        << "  --slots NUMBER          Conversation slots\n"
        << "  --category NAME         Knowledge category\n"
        << "  --goal NAME             spark, fail-spark, consecutive-spark,\n"
        << "                          consecutive-fail, accumulated-favor,\n"
        << "                          minimum-favor, or free-talk\n"
        << "  --target NUMBER         Goal target (use 0 for free-talk)\n\n"
        << "Other options:\n"
        << "  --list-categories       Print available categories\n"
        << "  -h, --help              Show this help\n";
}
