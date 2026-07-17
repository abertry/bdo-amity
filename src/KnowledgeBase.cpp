#include "KnowledgeBase.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <regex>
#include <set>
#include <stdexcept>

#ifndef KNOWLEDGE_DATABASE_PATH
#define KNOWLEDGE_DATABASE_PATH "data/knowledge.json"
#endif

std::unordered_map<std::string, Knowledge> KnowledgeBase::load(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("unable to open knowledge database: " + path);
    }
    const std::string json((std::istreambuf_iterator<char>(input)), {});
    const std::regex categoryPattern(
        R"json("([^"]+)"\s*:\s*\[([^\]]*)\])json"
    );
    const std::regex knowledgePattern(
        R"json(\{\s*"name"\s*:\s*"([^"]+)"\s*,\s*"interest"\s*:\s*(-?[0-9]+)\s*,\s*"favorMin"\s*:\s*(-?[0-9]+)\s*,\s*"favorMax"\s*:\s*(-?[0-9]+)\s*\})json"
    );
    std::unordered_map<std::string, Knowledge> result;
    for (auto categoryIt = std::sregex_iterator(json.begin(), json.end(), categoryPattern);
         categoryIt != std::sregex_iterator(); ++categoryIt) {
        const std::string category = (*categoryIt)[1].str();
        const std::string entries = (*categoryIt)[2].str();
        for (auto knowledgeIt = std::sregex_iterator(entries.begin(), entries.end(), knowledgePattern);
             knowledgeIt != std::sregex_iterator(); ++knowledgeIt) {
            Knowledge knowledge{(*knowledgeIt)[1].str(), std::stoi((*knowledgeIt)[2].str()),
                std::stoi((*knowledgeIt)[3].str()), std::stoi((*knowledgeIt)[4].str()), category};
            if (knowledge.favorMin > knowledge.favorMax) {
                throw std::runtime_error("invalid favor range for knowledge: " + knowledge.name);
            }
            if (!result.emplace(knowledge.name, knowledge).second) {
                throw std::runtime_error("duplicate knowledge name: " + knowledge.name);
            }
        }
    }
    if (result.empty()) {
        throw std::runtime_error("knowledge database contains no valid entries: " + path);
    }
    return result;
}

const std::unordered_map<std::string, Knowledge>& KnowledgeBase::all() {
    static const auto database = load(KNOWLEDGE_DATABASE_PATH);

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

std::vector<Knowledge> KnowledgeBase::selectCategory(const std::string& category) {
    std::vector<Knowledge> result;
    for (const auto& entry : all()) {
        if (entry.second.category == category) {
            result.push_back(entry.second);
        }
    }
    std::sort(result.begin(), result.end(), [](const Knowledge& a, const Knowledge& b) {
        return a.name < b.name;
    });
    return result;
}

std::vector<std::string> KnowledgeBase::categories() {
    std::set<std::string> unique;
    for (const auto& entry : all()) {
        unique.insert(entry.second.category);
    }
    return {unique.begin(), unique.end()};
}
