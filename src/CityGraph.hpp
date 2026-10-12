#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


class CityGraph {
    std::unordered_map<int, std::vector<std::pair<int, int>>> adj;
    std::unordered_map<int, std::string> names;

public:
    void addLocation(int id, const std::string& name);
    std::string nameOf(int id) const;
};

bool CityGraph::addRoad(int u, int v, int km) {
    if (!hasNode(u) || !hasNode(v) || km <= 0) {
        return false; 
    }
    
    
    for (const auto& neighbor : adj[u]) {
        if (neighbor.first == v) {
            return false; 
        }
    }

    adj[u].push_back({v, km});
    adj[v].push_back({u, km}); 
    return true;
}

size_t CityGraph::edgeCount() const {
    size_t total = 0;
    for (const auto& pair : adj) {
        total += pair.second.size();
    }
    return total / 2; 
}
