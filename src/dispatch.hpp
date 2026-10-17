#pragma once

#include <optional>
#include <queue>
#include <vector>

#include "auth.hpp"
#include "graph.hpp"

// One available driver evaluated for a pickup request.
struct DriverCandidate {
    int driverId = 0;
    int distanceKm = 0; 
    double rating = 0.0;
    double score = 0.0;  
}; 

//DriverPriorityQueue class
class DriverPriorityQueue {
    struct LowerPriority {
        bool operator()(const DriverCandidate& a, const DriverCandidate& b) const;
    };
    std::priority_queue<DriverCandidate, std::vector<DriverCandidate>, LowerPriority> pq;

public:
    void push(const DriverCandidate& c) { pq.push(c); }
    DriverCandidate pop();
    const DriverCandidate& top() const { return pq.top(); }
    bool empty() const { return pq.empty(); }
    size_t size() const { return pq.size(); }
};

//DriverPriorityQueue
class DriverPriorityQueue {
    struct LowerPriority {
        bool operator()(const DriverCandidate& a, const DriverCandidate& b) const;
    };
    std::priority_queue<DriverCandidate, std::vector<DriverCandidate>, LowerPriority> pq;

public:
    void push(const DriverCandidate& c) { pq.push(c); }
    DriverCandidate pop();
    const DriverCandidate& top() const { return pq.top(); }
    bool empty() const { return pq.empty(); }
    size_t size() const { return pq.size(); }
};
