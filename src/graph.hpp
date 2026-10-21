
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
    bool addRoad(int u, int v, int km); 

    bool hasNode(int id) const { return adj.count(id) > 0; }
    std::string nameOf(int id) const;
    std::vector<int> nodeIds() const; 
    const std::vector<std::pair<int, int>>& neighbours(int id) const;
    size_t nodeCount() const { return adj.size(); }
    size_t edgeCount() const;

    void loadDefaultDehradun();  
    void print() const;
};


struct RouteResult {
    bool found = false;
    int distanceKm = 0;
    std::vector<int> path; 
};

class RoutePlanner {
    const CityGraph& graph;

public:
    explicit RoutePlanner(const CityGraph& g) : graph(g) {}

    RouteResult shortestPath(int src, int dst) const;
   
    std::unordered_map<int, int> distancesFrom(int src) const;
    std::string describe(const RouteResult& r) const;  
};
