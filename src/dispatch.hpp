#pragma once

#include <optional>
#include <queue>
#include <vector>

#include "auth.hpp"
#include "graph.hpp"


struct DriverCandidate {
    int driverId = 0;
    int distanceKm = 0;   
    double rating = 0.0;
};

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

class DriverDispatcher {
    const AuthenticationManager& auth;
    const RoutePlanner& planner;

public:
    static constexpr double WEIGHT_DISTANCE = 0.6;
    static constexpr double WEIGHT_RATING = 0.4;

    DriverDispatcher(const AuthenticationManager& a, const RoutePlanner& p) : auth(a), planner(p) {}

    static double compositeScore(int distanceKm, double rating);
    DriverPriorityQueue buildQueue(int pickupNode) const;  
    std::optional<DriverCandidate> findBestDriver(int pickupNode) const;
    std::vector<DriverCandidate> topCandidates(int pickupNode, size_t k) const;
};
