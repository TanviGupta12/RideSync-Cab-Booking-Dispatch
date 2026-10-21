// =============================================================================
//  graph.hpp  --  RideSync : City Graph & Route Planning Module
//  Owner  : Member 2 (Jahnavi)
//  Classes: CityGraph (adjacency list), RoutePlanner (Dijkstra's algorithm)
// =============================================================================
#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// ----------------------------------------------------------------------------
// CityGraph : undirected weighted graph, weights are road distances in km.
// Representation: unordered_map<node, vector<pair<neighbour, km>>>
// (Member 2 - Jahnavi)
// ----------------------------------------------------------------------------
class CityGraph {
    std::unordered_map<int, std::vector<std::pair<int, int>>> adj;
    std::unordered_map<int, std::string> names;

public:
    void addLocation(int id, const std::string& name);
    bool addRoad(int u, int v, int km);  // false if a node is unknown or km <= 0

    bool hasNode(int id) const { return adj.count(id) > 0; }
    std::string nameOf(int id) const;
    std::vector<int> nodeIds() const;  // sorted ascending
    const std::vector<std::pair<int, int>>& neighbours(int id) const;
    size_t nodeCount() const { return adj.size(); }
    size_t edgeCount() const;

    void loadDefaultDehradun();  // illustrative Dehradun road network
    void print() const;
};

// ----------------------------------------------------------------------------
// RoutePlanner : Dijkstra's shortest path using a min-heap priority_queue
// Complexity O((V + E) log V)   (Member 2 - Jahnavi)
// ----------------------------------------------------------------------------
struct RouteResult {
    bool found = false;
    int distanceKm = 0;
    std::vector<int> path;  // node ids from source to destination
};

class RoutePlanner {
    const CityGraph& graph;

public:
    explicit RoutePlanner(const CityGraph& g) : graph(g) {}

    RouteResult shortestPath(int src, int dst) const;
    // single-source distances to every reachable node (used by the dispatcher)
    std::unordered_map<int, int> distancesFrom(int src) const;
    std::string describe(const RouteResult& r) const;  // "A -> B -> C"
};
