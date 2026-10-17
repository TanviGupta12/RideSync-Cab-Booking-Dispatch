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
