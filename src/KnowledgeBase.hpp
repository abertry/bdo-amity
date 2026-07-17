#pragma once

#include "Knowledge.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class KnowledgeBase {
public:
    static const std::unordered_map<std::string, Knowledge>& all();

    static std::unordered_map<std::string, Knowledge> load(const std::string& path);

    static std::optional<Knowledge> get(const std::string& name);

    static std::vector<Knowledge> select(const std::vector<std::string>& names);

    static std::vector<Knowledge> selectCategory(const std::string& category);

    static std::vector<std::string> categories();
};
