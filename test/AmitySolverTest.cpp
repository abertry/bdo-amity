#include "AmitySolver.hpp"
#include "CommandLine.hpp"
#include "KnowledgeBase.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear(double actual, double expected, const std::string& message) {
    expect(std::abs(actual - expected) < 1e-9, message);
}

void testProbabilityHelpers() {
    const Npc npc{"NPC", 20.0, 15, 2};
    expectNear(AmitySolver::sparkChance({"Low", 10.0, 20, 30, "Test"}, npc), 0.5,
        "spark chance uses knowledge/NPC interest");
    expectNear(AmitySolver::sparkChance({"High", 30.0, 20, 30, "Test"}, npc), 1.0,
        "spark chance is capped at one");
    expectNear(AmitySolver::expectedFavorGain({"Favor", 10.0, 20, 30, "Test"}, npc), 10.0,
        "expected favor uses the range average");
}

void testExactSparkSelection() {
    const Npc npc{"NPC", 100.0, 0, 1};
    const std::vector<Knowledge> available{
        {"Weak", 20.0, 100, 100, "Test"}, {"Reliable", 80.0, 1, 1, "Test"}
    };
    const auto result = AmitySolver::solveExact(available, {GoalType::Spark, 1}, npc);
    expect(result.order.size() == 1 && result.order[0].name == "Reliable",
        "goal probability takes precedence over expected reward");
    expectNear(result.satisfactionProbability, 0.8, "spark probability is exact");
    expectNear(result.expectedAccumulatedFavor, 0.8, "expected reward is reported");
}

void testExactFailSelection() {
    const Npc npc{"NPC", 100.0, 0, 1};
    const auto result = AmitySolver::solveExact({
        {"Usually fails", 10.0, 1, 1, "Test"}, {"Usually sparks", 90.0, 50, 50, "Test"}
    }, {GoalType::FailSpark, 1}, npc);
    expect(result.order[0].name == "Usually fails", "fail goal selects lowest spark chance");
    expectNear(result.satisfactionProbability, 0.9, "fail probability is exact");
}

void testConsecutiveOrderMatters() {
    const Npc npc{"NPC", 100.0, 0, 3};
    const auto result = AmitySolver::solveExact({
        {"Certain A", 100.0, 1, 1, "Test"}, {"Certain B", 100.0, 1, 1, "Test"},
        {"Always fail", 0.0, 100, 100, "Test"}
    }, {GoalType::ConsecutiveSpark, 2}, npc);
    expect(result.order[0].name != "Always fail" && result.order[1].name != "Always fail",
        "ordering places two guaranteed sparks consecutively");
    expectNear(result.satisfactionProbability, 1.0, "consecutive goal is guaranteed");
}

void testFavorDistribution() {
    const Npc npc{"NPC", 100.0, 10, 1};
    const auto result = AmitySolver::solveExact({{"Range", 100.0, 10, 12, "Test"}},
        {GoalType::AccumulatedFavor, 2}, npc);
    expectNear(result.satisfactionProbability, 1.0 / 3.0,
        "goal probability evaluates every favor-range outcome");
    expectNear(result.expectedAccumulatedFavor, 1.0, "favor expectation is exact");
}

void testRewardAndBranchBound() {
    const Npc npc{"NPC", 100.0, 0, 2};
    const auto result = AmitySolver::solveExact({
        {"One", 100.0, 1, 1, "Test"}, {"Ten", 100.0, 10, 10, "Test"},
        {"Five", 100.0, 5, 5, "Test"}
    }, {GoalType::FreeTalk, 0}, npc);
    expect(result.order.size() == 2, "selection respects conversation slots");
    expectNear(result.expectedAccumulatedFavor, 15.0, "best reward selection is exact");
    expect(result.prunedBranches > 0, "branch-and-bound prunes dominated branches");
}

void testJsonKnowledgeBase() {
    const auto& all = KnowledgeBase::all();
    expect(all.size() == 26, "all JSON knowledge records load");
    const auto igor = KnowledgeBase::get("Igor Bartali");
    expect(igor && igor->favorMin == 45 && igor->category == "Velia Residents",
        "canonical categorized entry comes from JSON");
    expect(KnowledgeBase::selectCategory("Velia Residents").size() == 14,
        "Velia Residents category contains all expected entries");
    expect(KnowledgeBase::categories().size() == 2, "available categories are listed");

    const std::string path = "invalid-knowledge-test.json";
    { std::ofstream output(path); output << "not json"; }
    bool threw = false;
    try { (void)KnowledgeBase::load(path); }
    catch (const std::runtime_error&) { threw = true; }
    std::remove(path.c_str());
    expect(threw, "malformed database is rejected");
}

void testValidationAndEmptyInput() {
    const auto empty = AmitySolver::solveExact({}, {GoalType::FreeTalk, 0},
        {"NPC", 1, 0, 0});
    expect(empty.order.empty() && empty.satisfactionProbability == 1.0,
        "empty free-talk conversation is valid");
    bool threw = false;
    try {
        (void)AmitySolver::solveExact({{"Bad", 1, 5, 4, "Test"}},
            {GoalType::Spark, 1}, {"NPC", 1, 0, 1});
    } catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "invalid favor ranges are rejected");
}

void testCommandLineParsing() {
    const char* arguments[]{"solver", "--interest", "30", "--favor", "15", "--slots", "8",
        "--category", "Velia Residents", "--goal", "consecutive-spark", "--target", "3"};
    const auto options = parseCommandLine(13, arguments);
    expect(options.npc.interest == 30.0 && options.npc.favor == 15 && options.npc.slots == 8,
        "NPC command-line values are parsed");
    expect(options.category == "Velia Residents", "category command-line value is parsed");
    expect(options.goal.type == GoalType::ConsecutiveSpark && options.goal.target == 3,
        "goal command-line values are parsed");

    const char* invalid[]{"solver", "--interest", "30"};
    bool threw = false;
    try { (void)parseCommandLine(3, invalid); }
    catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "missing required command-line options are rejected");
}
}

int main() {
    testProbabilityHelpers();
    testExactSparkSelection();
    testExactFailSelection();
    testConsecutiveOrderMatters();
    testFavorDistribution();
    testRewardAndBranchBound();
    testJsonKnowledgeBase();
    testValidationAndEmptyInput();
    testCommandLineParsing();
    if (failures == 0) {
        std::cout << "All unit tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
