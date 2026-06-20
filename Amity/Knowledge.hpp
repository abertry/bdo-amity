#pragma once

#include <string>

struct Knowledge {
    std::string name;
    double interest;
    int favorMin;
    int favorMax;

    double avgFavor() const {
        return (favorMin + favorMax) / 2.0;
    }
};
