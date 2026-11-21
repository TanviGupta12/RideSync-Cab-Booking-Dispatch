
#pragma once

#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

#include "auth.hpp"
#include "dispatch.hpp"
#include "graph.hpp"

struct Booking {
    int id = 0;
    int customerId = 0;
    int driverId = 0;       // 0 while the booking is PENDING
    int pickup = 0;
    int drop = 0;
    int distanceKm = 0;     // pickup -> drop (Dijkstra)
    int pickupDistKm = 0;   // driver -> pickup
    double fare = 0.0;
    std::string status = "PENDING";  // PENDING | CONFIRMED | COMPLETED | CANCELLED
    double rating = 0.0;    // 0 = not rated yet
    std::string timestamp;
};


class FareCalculator {
    double baseFare, perKm, minFare;

public:
    explicit FareCalculator(double base = 40.0, double perKm = 12.0, double minFare = 60.0)
        : baseFare(base), perKm(perKm), minFare(minFare) {}
    double calculate(int distanceKm, double surge = 1.0) const;
};


class BookingManager {
    AuthenticationManager& auth;
    const RoutePlanner& planner;
    const DriverDispatcher& dispatcher;
    FareCalculator fareCalc;

    std::vector<Booking> bookings;
    std::queue<int> pending;                            // booking ids
    std::unordered_map<int, std::stack<int>> recent;    // customerId -> booking ids
    int nextId = 1;

    Booking* find(int bookingId);
    bool assign(Booking& b);

public:
    BookingManager(AuthenticationManager& a, const RoutePlanner& p, const DriverDispatcher& d,
                   FareCalculator f = FareCalculator())
        : auth(a), planner(p), dispatcher(d), fareCalc(f) {}

   
    bool bookRide(int customerId, int pickup, int drop, Booking& out, std::string& err);
    bool completeRide(int driverId, Booking& out, std::string& err);
    bool cancelBooking(int bookingId, int customerId, std::string& err);
    int cancelLastBooking(int customerId, std::string& err);  // booking id, or -1
    bool rateRide(int bookingId, int customerId, double stars, std::string& err);
    int processPending();  // assigns waiting requests to freed drivers
    double quoteFare(int distanceKm) const;  // fare incl. current demand surge (used by bookRide and the website)

    std::vector<Booking> historyOf(int customerId) const;
    const Booking* activeBookingOfDriver(int driverId) const;
    const std::vector<Booking>& all() const { return bookings; }
    size_t pendingCount() const { return pending.size(); }
    void load(std::vector<Booking> loaded);  // rebuilds queue and stacks
};


class FileStorageManager {
    std::string usersFile, bookingsFile;

public:
    explicit FileStorageManager(std::string users = "users.txt", std::string bookings = "bookings.txt")
        : usersFile(std::move(users)), bookingsFile(std::move(bookings)) {}

    bool saveUsers(const AuthenticationManager& auth) const;
    bool saveBookings(const BookingManager& bm) const;
    void loadUsers(AuthenticationManager& auth) const;
    void loadBookings(BookingManager& bm) const;
};
