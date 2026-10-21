// =============================================================================
//  dispatch.hpp  --  RideSync : Driver Dispatch Module
//  Owner  : Member 4 (Dhairya Sharma)
//  Classes: DriverPriorityQueue, DriverDispatcher
// =============================================================================
#pragma once

#include <optional>
#include <queue>
#include <vector>

#include "auth.hpp"
#include "graph.hpp"

// One available driver evaluated for a pickup request.
struct DriverCandidate {
    int driverId = 0;
    int distanceKm = 0;   // shortest-path distance driver -> pickup
    double rating = 0.0;
    double score = 0.0;   // composite score, higher is better
};

// ----------------------------------------------------------------------------
// DriverPriorityQueue : max-heap (std::priority_queue) ordered by composite
// score; ties are broken in favour of the smaller driver id.
// (Member 4 - Dhairya Sharma)
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// DriverDispatcher : score = 0.6 * 1/(1 + distanceKm) + 0.4 * rating/5
// Proximity gets the larger weight; rating separates drivers at similar range.
// (Member 4 - Dhairya Sharma)
// ----------------------------------------------------------------------------
class DriverDispatcher {
    const AuthenticationManager& auth;
    const RoutePlanner& planner;

public:
    static constexpr double WEIGHT_DISTANCE = 0.6;
    static constexpr double WEIGHT_RATING = 0.4;

    DriverDispatcher(const AuthenticationManager& a, const RoutePlanner& p) : auth(a), planner(p) {}

    static double compositeScore(int distanceKm, double rating);
    DriverPriorityQueue buildQueue(int pickupNode) const;  // available + reachable drivers only
    std::optional<DriverCandidate> findBestDriver(int pickupNode) const;
    std::vector<DriverCandidate> topCandidates(int pickupNode, size_t k) const;
};
