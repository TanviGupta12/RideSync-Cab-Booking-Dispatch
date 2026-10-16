
#include "graph.hpp"

#include <algorithm>
#include <iostream>
#include <functional>
#include <limits>
#include <queue>
#include <sstream>
#include <utility>

void CityGraph::addLocation(int id, const std::string& name) {
    names[id] = name;
    adj.try_emplace(id);
}

bool CityGraph::addRoad(int u, int v, int km) {
    if (!hasNode(u) || !hasNode(v) || u == v || km <= 0) {
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

std::string CityGraph::nameOf(int id) const {
    const auto it = names.find(id);

    if (it == names.end()) {
        return "Unknown location";
    }

    return it->second;
}

std::vector<int> CityGraph::nodeIds() const {
    std::vector<int> ids;
    ids.reserve(adj.size());

    for (const auto& entry : adj) {
        ids.push_back(entry.first);
    }

    std::sort(ids.begin(), ids.end());
    return ids;
}

const std::vector<std::pair<int, int>>&
CityGraph::neighbours(int id) const {
    static const std::vector<std::pair<int, int>> empty;

    const auto it = adj.find(id);

    if (it == adj.end()) {
        return empty;
    }

    return it->second;
}

std::size_t CityGraph::nodeCount() const {
    return adj.size();
}

std::size_t CityGraph::edgeCount() const {
    std::size_t total = 0;

    for (const auto& entry : adj) {
        total += entry.second.size();
    }

    return total / 2;
}
