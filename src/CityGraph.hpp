#ifndef CITYGRAPH_HPP
#define CITYGRAPH_HPP

#include <vector>
#include <unordered_map>

// Represents a road connecting two locations with a distance weight
struct Edge {
    int destination;
    int weight;
};

#endif
