#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

struct RouteResult {
    bool found = false;
    int distanceKm = 0;
    std::vector<int> path;
};

class CityGraph {
private:
    std::unordered_map<int, std::vector<std::pair<int, int>>> adj;
    std::unordered_map<int, std::string> names;

public:
    void addLocation(int id, const std::string& name);
    bool addRoad(int u, int v, int km);

    bool hasNode(int id) const;
    std::string nameOf(int id) const;

    std::vector<int> nodeIds() const;
    const std::vector<std::pair<int, int>>& neighbours(int id) const;

    std::size_t nodeCount() const;
    std::size_t edgeCount() const;

    void loadDefaultDehradun();
    void print() const;
};

class RoutePlanner {
private:
    const CityGraph& graph;

public:
    explicit RoutePlanner(const CityGraph& graph);

    RouteResult shortestPath(int start, int goal) const;
    std::unordered_map<int, int> distancesFrom(int start) const;
    std::string describe(const RouteResult& result) const;
};
