#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


class CityGraph {
    std::unordered_map<int, std::vector<std::pair<int, int>>> adj;
    std::unordered_map<int, std::string> names;

public:
    // --- Commit 2 Addition: Location Management Declarations ---
    void addLocation(int id, const std::string& name);
    std::string nameOf(int id) const;
};
