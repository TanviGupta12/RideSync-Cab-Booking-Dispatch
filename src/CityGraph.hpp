#ifndef CITYGRAPH_HPP
#define CITYGRAPH_HPP

#include <vector>
#include <unordered_map>
#include <iostream>

// Graph Edge representation
struct Edge {
    int destination;
    int weight;
};

// City Map Graph using Adjacency List
class CityGraph {
private:
    std::unordered_map<int, std::vector<Edge>> adjList;

public:
    void addNode(int nodeId);
    void addEdge(int src, int dest, int weight, bool bidirectional = true);
    bool nodeExists(int nodeId) const;

    // ---> THIS IS WHAT WE ARE ADDING FOR COMMIT 2 <---
    // Dijkstra's Algorithm function signature
    std::pair<int, std::vector<int>> getShortestPath(int startNode, int endNode);
};

#endif
