
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

void CityGraph::loadDefaultDehradun() {
    // Clear existing graph data before loading the default map.
    adj.clear();
    names.clear();

    // Add Dehradun locations.
    addLocation(1, "Clock Tower");
    addLocation(2, "Parade Ground");
    addLocation(3, "Railway Station");
    addLocation(4, "ISBT");
    addLocation(5, "Forest Research Institute");
    addLocation(6, "Indian Military Academy");
    addLocation(7, "Pacific Mall");
    addLocation(8, "Sahastradhara");
    addLocation(9, "Prem Nagar");
    addLocation(10, "DIT University");

    // Add road connections and distances in kilometres.
    addRoad(1, 2, 1);
    addRoad(1, 3, 3);
    addRoad(1, 7, 6);
    addRoad(1, 8, 8);
    addRoad(2, 3, 2);
    addRoad(2, 4, 6);
    addRoad(3, 4, 5);
    addRoad(3, 5, 5);
    addRoad(4, 6, 8);
    addRoad(5, 6, 4);
    addRoad(6, 9, 3);
    addRoad(7, 8, 7);
    addRoad(8, 10, 12);
}

void CityGraph::print() const {
    for (int id : nodeIds()) {
        std::cout << "  [" << id << "] " << nameOf(id) << "\n";
        for (const auto& e : neighbours(id))
            std::cout << "        -> " << nameOf(e.first) << " (" << e.second << " km)\n";
    }
}

RouteResult RoutePlanner::shortestPath(int src, int dst) const {
    RouteResult res;
    if (!graph.hasNode(src) || !graph.hasNode(dst)) return res;

    const int INF = std::numeric_limits<int>::max();
    std::unordered_map<int, int> dist, parent;
    for (int id : graph.nodeIds()) dist[id] = INF;

    using State = std::pair<int, int>;  // (distance, node)
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;  // min-heap
    dist[src] = 0;
    parent[src] = -1;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;  // stale entry
        if (u == dst) break;        // early exit: dst finalised
        for (const auto& [v, w] : graph.neighbours(u)) {
            if (d + w < dist[v]) {
                dist[v] = d + w;
                parent[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    if (dist[dst] == INF) return res;  // unreachable
    res.found = true;
    res.distanceKm = dist[dst];
    for (int at = dst; at != -1; at = parent[at]) res.path.push_back(at);
    std::reverse(res.path.begin(), res.path.end());
    return res;
}

std::unordered_map<int, int> RoutePlanner::distancesFrom(int src) const {
    std::unordered_map<int, int> dist;
    if (!graph.hasNode(src)) return dist;

    using State = std::pair<int, int>;
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;
    dist[src] = 0;
    pq.push({0, src});
    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;
        for (const auto& [v, w] : graph.neighbours(u)) {
            auto it = dist.find(v);
            if (it == dist.end() || d + w < it->second) {
                dist[v] = d + w;
                pq.push({dist[v], v});
            }
        }
    }
    return dist;
}
