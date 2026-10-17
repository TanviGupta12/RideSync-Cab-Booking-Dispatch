#include "dispatch.hpp"

bool DriverPriorityQueue::LowerPriority::operator()(const DriverCandidate& a,
                                                    const DriverCandidate& b) const {
    if (a.score != b.score) return a.score < b.score;
    return a.driverId > b.driverId;
}

DriverCandidate DriverPriorityQueue::pop() {
    DriverCandidate best = pq.top();
    pq.pop();
    return best;
}
double DriverDispatcher::compositeScore(int distanceKm, double rating) {
    return WEIGHT_DISTANCE * (1.0 / (1.0 + distanceKm)) + WEIGHT_RATING * (rating / 5.0);
}
