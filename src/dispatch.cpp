#include "dispatch.hpp"

bool DriverPriorityQueue::LowerPriority::operator()(const DriverCandidate& a,
                                                    const DriverCandidate& b) const {
    if (a.score != b.score) return a.score < b.score;
    return a.driverId > b.driverId;
}

DriverCandidate DriverPriorityQueue::pop() 
{
    DriverCandidate best = pq.top();
    pq.pop();
    return best;
}
double DriverDispatcher::compositeScore(int distanceKm, double rating)
{
    return WEIGHT_DISTANCE * (1.0 / (1.0 + distanceKm)) + WEIGHT_RATING * (rating / 5.0);
}
DriverPriorityQueue DriverDispatcher::buildQueue(int pickupNode) const 
{
    DriverPriorityQueue queue;
  
    // One Dijkstra run from the pickup gives the distance to every driver.
    const auto dist = planner.distancesFrom(pickupNode);
    for (const auto& driver : auth.allDrivers())
      {
        if (!driver->isAvailable()) continue;
        auto it = dist.find(driver->getLocation());
        if (it == dist.end()) continue;  // driver cannot reach the pickup
        DriverCandidate c;
        c.driverId = driver->getId();
        c.distanceKm = it->second;
        c.rating = driver->getRating();
        c.score = compositeScore(c.distanceKm, c.rating);
        queue.push(c);
    }
    return queue; //queue return
}

std::optional<DriverCandidate> DriverDispatcher::findBestDriver(int pickupNode) const
{
    auto queue = buildQueue(pickupNode);
    if (queue.empty()) return std::nullopt;
    return queue.pop();
}
 
std::vector<DriverCandidate> DriverDispatcher::topCandidates(int pickupNode, size_t k) const 
{
    auto queue = buildQueue(pickupNode);
    std::vector<DriverCandidate> out;
    while (!queue.empty() && out.size() < k) out.push_back(queue.pop());
    return out;
}
