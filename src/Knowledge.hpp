#pragma once

#include <string>

struct Knowledge {
    std::string name;
    double interest;
    int favorMin;
    int favorMax;
    std::string category;

    double avgFavor() const {
        return (favorMin + favorMax) / 2.0;
    }
};
