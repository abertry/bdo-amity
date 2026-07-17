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
    const Npc npc{20.0, 15, 2};
    expectNear(AmitySolver::sparkChance({"Low", 10, 20, 30, "Test"}, npc), 0.5,
        "spark chance uses knowledge/NPC interest");
    expectNear(AmitySolver::sparkChance({"High", 30, 20, 30, "Test"}, npc), 1.0,
        "spark chance is capped at one");
    expectNear(AmitySolver::expectedFavorGain({"Favor", 10, 20, 30, "Test"}, npc), 10.0,
        "expected favor uses the range average");
    expectNear(AmitySolver::expectedFavorGain({"Minimum", 10, 10, 10, "Test"}, npc), 1.0,
        "successful topics award at least one favor");
}

void testExactSparkSelection() {
    const Npc npc{100.0, 0, 1};
    const std::vector<Knowledge> available{
        {"Weak", 20, 100, 100, "Test"}, {"Reliable", 80, 1, 1, "Test"}
    };
    const auto result = AmitySolver::solveExact(available, {GoalType::Spark, 1}, npc);
    expect(result.order.size() == 1 && result.order[0].name == "Reliable",
        "goal probability takes precedence over expected reward");
    expectNear(result.satisfactionProbability, 0.8, "spark probability is exact");
    expectNear(result.expectedAccumulatedFavor, 0.8, "expected reward is reported");
}

void testExactFailSelection() {
    const Npc npc{100.0, 0, 1};
    const auto result = AmitySolver::solveExact({
        {"Usually fails", 10, 1, 1, "Test"}, {"Usually sparks", 90, 50, 50, "Test"}
    }, {GoalType::FailSpark, 1}, npc);
    expect(result.order[0].name == "Usually fails", "fail goal selects lowest spark chance");
    expectNear(result.satisfactionProbability, 0.9, "fail probability is exact");
}

void testConsecutiveOrderMatters() {
    const Npc npc{100.0, 0, 3};
    const auto result = AmitySolver::solveExact({
        {"Certain A", 100, 1, 1, "Test"}, {"Certain B", 100, 1, 1, "Test"},
        {"Always fail", 0, 100, 100, "Test"}
    }, {GoalType::ConsecutiveSpark, 2}, npc);
    expect(result.order[0].name != "Always fail" && result.order[1].name != "Always fail",
        "ordering places two guaranteed sparks consecutively");
    expectNear(result.satisfactionProbability, 1.0, "consecutive goal is guaranteed");
}

void testFavorDistribution() {
    const Npc npc{100.0, 10, 1};
    const auto result = AmitySolver::solveExact({{"Range", 100, 10, 12, "Test"}},
        {GoalType::AccumulatedFavor, 2}, npc);
    expectNear(result.satisfactionProbability, 1.0 / 3.0,
        "goal probability evaluates every favor-range outcome");
    expectNear(result.expectedAccumulatedFavor, 4.0 / 3.0, "minimum-one favor expectation is exact");
}

void testObservedFavorAccumulation() {
    const Npc npc{1.0, 0, 5};
    const std::vector<Knowledge> goodRun{
        {"Feinia", 1, 15, 15, "Test"}, {"Hessenvale", 1, 6, 6, "Test"},
        {"Constant", 1, 6, 6, "Test"}, {"Samuel", 1, 5, 5, "Test"},
        {"Claus", 1, 0, 0, "Test"}
    };
    const auto result = AmitySolver::evaluateOrder(goodRun, {GoalType::FreeTalk, 0}, npc);
    expectNear(result.expectedMaximumFavor, 33.0,
        "Reddit good run produces observed maximum favor");
    expectNear(result.expectedAccumulatedFavor, 128.0,
        "Reddit good run produces observed accumulated favor");
}

void testFailureResetsMaximumFavor() {
    const Npc npc{100.0, 0, 3};
    const std::vector<Knowledge> order{
        {"Spark", 100, 10, 10, "Test"}, {"Fail", 0, 100, 100, "Test"},
        {"Spark again", 100, 5, 5, "Test"}
    };
    const auto result = AmitySolver::evaluateOrder(order, {GoalType::FreeTalk, 0}, npc);
    expectNear(result.expectedMaximumFavor, 5.0, "failure resets maximum favor");
    expectNear(result.expectedAccumulatedFavor, 15.0,
        "accumulated favor survives failure while failed turn adds zero");
}

void testObservedFavorCombo() {
    const Npc npc{1.0, 29, 5};
    const ComboEffect reduceNpcFavor{ComboEffectType::NpcFavor, 2, 4, -4};
    const std::vector<Knowledge> clausRun{
        {"Claus", 1, 29, 29, "Test", {reduceNpcFavor}},
        {"Feinia", 1, 40, 40, "Test"}, {"Constant", 1, 36, 36, "Test"},
        {"Samuel", 1, 34, 34, "Test"}, {"Hessenvale", 1, 37, 37, "Test"}
    };
    const auto result = AmitySolver::evaluateOrder(clausRun, {GoalType::FreeTalk, 0}, npc);
    expectNear(result.expectedMaximumFavor, 44.0,
        "delayed NPC favor combo produces observed maximum favor");
    expectNear(result.expectedAccumulatedFavor, 112.0,
        "delayed NPC favor combo produces observed accumulated favor");
}

void testExactRewardOrdering() {
    const Npc npc{100.0, 0, 2};
    const auto result = AmitySolver::solveExact({
        {"One", 100, 1, 1, "Test"}, {"Ten", 100, 10, 10, "Test"},
        {"Five", 100, 5, 5, "Test"}
    }, {GoalType::FreeTalk, 0}, npc);
    expect(result.order.size() == 2, "selection respects conversation slots");
    expectNear(result.expectedAccumulatedFavor, 25.0,
        "best reward selection and high-first ordering are exact");
    expect(result.order[0].name == "Ten", "highest favor is placed first for accumulated favor");
    expect(result.prunedBranches > 0, "admissible accumulated-favor bound prunes branches");
}

void testLargeCertainConsecutiveSearchPrunes() {
    const Npc npc{15.0, 30, 8};
    std::vector<Knowledge> available;
    for (int index = 0; index < 14; ++index) {
        available.push_back({"Topic " + std::to_string(index), 15 + index,
            30 + index, 31 + index, "Test"});
    }
    const auto result = AmitySolver::solveExact(
        available, {GoalType::ConsecutiveSpark, 3}, npc);
    expectNear(result.satisfactionProbability, 1.0,
        "large certain consecutive-spark goal remains exact");
    expect(result.prunedBranches > 0, "large certain search prunes dominated branches");
    expect(result.exploredOrders < 100000,
        "large certain search avoids factorial order enumeration");
}

void testReportedCategorySearchShortcuts() {
    const auto result = AmitySolver::solveExact(
        KnowledgeBase::selectCategory("Residents of Velia"),
        {GoalType::ConsecutiveSpark, 3}, {15.0, 30, 8});
    expectNear(result.satisfactionProbability, 1.0,
        "reported category consecutive-spark goal remains exact");
    expect(result.exploredOrders < 100000,
        "reported category avoids factorial order enumeration");
}

void testJsonKnowledgeBase() {
    const auto& all = KnowledgeBase::all();
    expect(all.size() == 26, "all JSON knowledge records load");
    const auto igor = KnowledgeBase::get("Igor Bartali");
    expect(igor && igor->favorMin == 45 && igor->category == "Residents of Velia",
        "canonical categorized entry comes from JSON");
    expect(KnowledgeBase::selectCategory("Residents of Velia").size() == 14,
        "Residents of Velia category contains all expected entries");
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
        {1, 0, 0});
    expect(empty.order.empty() && empty.satisfactionProbability == 1.0,
        "empty free-talk conversation is valid");
    bool threw = false;
    try {
        (void)AmitySolver::solveExact({{"Bad", 1, 5, 4, "Test"}},
            {GoalType::Spark, 1}, {1, 0, 1});
    } catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "invalid favor ranges are rejected");
    threw = false;
    try {
        (void)AmitySolver::evaluateOrder(
            {{"Bad combo", 1, 1, 1, "Test", {{ComboEffectType::NpcFavor, 0, 1, -4}}}},
            {GoalType::FreeTalk, 0}, {1, 0, 1});
    } catch (const std::invalid_argument&) { threw = true; }
    expect(threw, "invalid combo timing is rejected");
}

void testCommandLineParsing() {
    const char* arguments[]{"solver", "--interest", "30", "--favor", "15", "--slots", "8",
        "--category", "Residents of Velia", "--goal", "consecutive-spark", "--target", "3"};
    const auto options = parseCommandLine(13, arguments);
    expect(options.npc.interest == 30.0 && options.npc.favor == 15 && options.npc.slots == 8,
        "NPC command-line values are parsed");
    expect(options.category == "Residents of Velia", "category command-line value is parsed");
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
    testObservedFavorAccumulation();
    testFailureResetsMaximumFavor();
    testObservedFavorCombo();
    testExactRewardOrdering();
    testLargeCertainConsecutiveSearchPrunes();
    testReportedCategorySearchShortcuts();
    testJsonKnowledgeBase();
    testValidationAndEmptyInput();
    testCommandLineParsing();
    if (failures == 0) {
        std::cout << "All unit tests passed\n";
    }
    return failures == 0 ? 0 : 1;
}
