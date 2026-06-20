#pragma once

enum class GoalType {
    Spark,
    FailSpark,
    ConsecutiveSpark,
    ConsecutiveFail,
    AccumulatedFavor,
    MinimumFavor,
    FreeTalk
};

struct Goal {
    GoalType type;
    int target;
};
