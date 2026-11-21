
#include "graph.hpp"

#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include <queue>


void CityGraph::addLocation(int id, const std::string& name) {
    adj[id];  
    names[id] = name;
}

bool CityGraph::addRoad(int u, int v, int km) {
    if (km <= 0 || u == v || !hasNode(u) || !hasNode(v)) return false;
    adj[u].push_back({v, km});
    adj[v].push_back({u, km});
    return true;
}

std::string CityGraph::nameOf(int id) const {
    auto it = names.find(id);
    return it == names.end() ? "Unknown" : it->second;
}

std::vector<int> CityGraph::nodeIds() const {
    std::vector<int> ids;
    for (const auto& kv : adj) ids.push_back(kv.first);
    std::sort(ids.begin(), ids.end());
    return ids;
}

const std::vector<std::pair<int, int>>& CityGraph::neighbours(int id) const {
    static const std::vector<std::pair<int, int>> empty;
    auto it = adj.find(id);
    return it == adj.end() ? empty : it->second;
}

size_t CityGraph::edgeCount() const {
    size_t total = 0;
    for (const auto& kv : adj) total += kv.second.size();
    return total / 2;  
}

void CityGraph::loadDefaultDehradun() {
   
    addLocation(1, "Graphic Era University");
    addLocation(2, "ISBT Dehradun");
    addLocation(3, "Clock Tower");
    addLocation(4, "Railway Station");
    addLocation(5, "Rajpur Road");
    addLocation(6, "Ballupur Chowk");
    addLocation(7, "Prem Nagar");
    addLocation(8, "Rispana Bridge");
    addLocation(9, "Sahastradhara Road");
    addLocation(10, "Forest Research Institute");

    addRoad(1, 6, 2);   addRoad(1, 7, 5);   addRoad(6, 8, 4);
    addRoad(6, 10, 6);  addRoad(7, 10, 3);  addRoad(7, 2, 8);
    addRoad(8, 3, 2);   addRoad(8, 4, 4);   addRoad(3, 4, 1);
    addRoad(3, 5, 3);   addRoad(2, 4, 3);   addRoad(5, 9, 7);
    addRoad(10, 5, 5);
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

    using State = std::pair<int, int>; 
    std::priority_queue<State, std::vector<State>, std::greater<State>> pq;  
    dist[src] = 0;
    parent[src] = -1;
    pq.push({0, src});

    while (!pq.empty()) {
        auto [d, u] = pq.top();
        pq.pop();
        if (d > dist[u]) continue;  
        if (u == dst) break;        
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

std::string RoutePlanner::describe(const RouteResult& r) const {
    std::string s;
    for (size_t i = 0; i < r.path.size(); ++i) {
        if (i) s += " -> ";
        s += graph.nameOf(r.path[i]);
    }
    return s;
}
