#include "KnowledgeBase.hpp"

#include <iostream>

const std::unordered_map<std::string, Knowledge>& KnowledgeBase::all() {
    static const std::unordered_map<std::string, Knowledge> database = {
        {"Graule", {"Graule", 4, 45, 57}},
        {"Luicy", {"Luicy", 3, 40, 53}},
        {"Buta", {"Buta", 34, 37, 41}},
        {"Yasum", {"Yasum", 17, 45, 47}},
        {"Ami", {"Ami", 44, 5, 7}},
        {"Dudora Doriven", {"Dudora Doriven", 20, 45, 49}},
        {"Roroju", {"Roroju", 2, 44, 46}},
        {"Sankamu", {"Sankamu", 43, 5, 8}},
        {"Larina", {"Larina", 7, 38, 48}},
        {"Rohu", {"Rohu", 25, 24, 28}},
        {"Buroma", {"Buroma", 32, 36, 42}},
        {"Surondula", {"Surondula", 25, 25, 26}},

        {"Islin Bartali", {"Islin Bartali", 21, 33, 36}},
        {"Crio", {"Crio", 10, 31, 38}},
        {"David Finto", {"David Finto", 35, 32, 39}},
        {"Alustin", {"Alustin", 25, 22, 28}},
        {"Igor Bartali", {"Igor Bartali", 20, 45, 47}},
        {"Artemio Fiazza", {"Artemio Fiazza", 41, 5, 9}},
        {"Claire Arryn", {"Claire Arryn", 4, 42, 53}},
        {"Abelin", {"Abelin", 33, 35, 42}},
        {"Mariano", {"Mariano", 20, 34, 37}},
        {"Egrin", {"Egrin", 35, 30, 40}},
        {"Hans", {"Hans", 28, 23, 26}},
        {"Dalishain", {"Dalishain", 21, 34, 37}},
        {"Houtman", {"Houtman", 23, 34, 39}},
        {"Momo", {"Momo", 29, 23, 30}},
    };

    return database;
}

std::optional<Knowledge> KnowledgeBase::get(const std::string& name) {
    const auto& database = all();

    auto it = database.find(name);
    if (it == database.end()) {
        return std::nullopt;
    }

    return it->second;
}

std::vector<Knowledge> KnowledgeBase::select(const std::vector<std::string>& names) {
    std::vector<Knowledge> result;

    for (const auto& name : names) {
        auto knowledge = get(name);

        if (knowledge.has_value()) {
            result.push_back(*knowledge);
        }
        else {
            std::cerr << "Unknown knowledge: " << name << "\n";
        }
    }

    return result;
}
