#pragma once

#include <string>
#include <vector>

enum class ComboEffectType {
    NpcFavor,
    NpcInterest,
    TopicFavor,
    TopicInterest
};

struct ComboEffect {
    ComboEffectType type;
    int delay;
    int duration;
    int amount;
};

struct Knowledge {
    std::string name;
    int interest;
    int favorMin;
    int favorMax;
    std::string category;
    std::vector<ComboEffect> comboEffects;

    double avgFavor() const {
        return (favorMin + favorMax) / 2.0;
    }
};
