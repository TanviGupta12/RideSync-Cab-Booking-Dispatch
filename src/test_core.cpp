// tests/test_core.cpp -- RideSync Phase-2 unit/integration tests (no framework needed)
// Build (from repo root):
//   g++ -std=c++17 -Wall -Wextra -I. -o test_core tests/test_core.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
#include <cstdio>
#include <iostream>
#include <string>

#include "auth.hpp"
#include "dispatch.hpp"
#include "graph.hpp"
#include "system.hpp"

static int passed = 0, failed = 0;
#define CHECK(cond)                                                              \
    do {                                                                         \
        if (cond) { ++passed; }                                                  \
        else { ++failed; std::cout << "  FAIL line " << __LINE__ << ": " #cond "\n"; } \
    } while (0)

// Fresh world: default map + 3 drivers (ids 1,2,3) + 1 customer (id 4)
struct Env {
    CityGraph city; RoutePlanner planner; AuthenticationManager auth;
    DriverDispatcher dispatcher; BookingManager bm;
    std::string err;
    Env() : city(), planner(city), auth(), dispatcher(auth, planner), bm(auth, planner, dispatcher) {
        city.loadDefaultDehradun();
    }
    void addDrivers() {
        auth.registerDriver("D One", "9000000001", "pass", "Car1", 3, err);  // Clock Tower
        auth.registerDriver("D Two", "9000000002", "pass", "Car2", 1, err);  // GEU
        auth.registerDriver("D Three", "9000000003", "pass", "Car3", 4, err);
    }
    int addCustomer() { return auth.registerCustomer("Cust", "8000000001", "pass", 1, err)->getId(); }
};

static void testGraph() {            // Jahnavi's module
    std::cout << "[graph]\n";
    Env e;
    CHECK(e.city.nodeCount() == 10);
    CHECK(e.city.edgeCount() == 13);
    CHECK(e.planner.shortestPath(1, 10).found && e.planner.shortestPath(1, 10).distanceKm == 8);
    auto r = e.planner.shortestPath(1, 9);
    CHECK(r.found && r.distanceKm == 18 && r.path.front() == 1 && r.path.back() == 9);
    CHECK(e.planner.shortestPath(1, 1).distanceKm == 0);
    CHECK(!e.planner.shortestPath(1, 99).found);                 // unknown node
    e.city.addLocation(99, "Island");
    CHECK(!e.planner.shortestPath(1, 99).found);                 // disconnected node
    CHECK(!e.city.addRoad(1, 1, 5) && !e.city.addRoad(1, 2, 0) && !e.city.addRoad(1, 77, 3));
    CHECK(e.planner.distancesFrom(1).at(10) == 8);
    CHECK(e.planner.distancesFrom(1).count(99) == 0);
}

static void testAuth() {             // Tanvi / Nishtha
    std::cout << "[auth]\n";
    Env e;
    CHECK(e.auth.registerCustomer("A", "8000000001", "pass", 1, e.err) != nullptr);
    CHECK(e.auth.registerCustomer("B", "8000000001", "pass", 1, e.err) == nullptr);   // duplicate phone
    CHECK(e.auth.registerCustomer("B", "123", "pass", 1, e.err) == nullptr);          // bad phone
    CHECK(e.auth.registerCustomer("B", "8000000002", "abc", 1, e.err) == nullptr);    // short password
    CHECK(e.auth.registerCustomer("", "8000000003", "pass", 1, e.err) == nullptr);    // empty name
    CHECK(e.auth.login("8000000001", "pass") != nullptr);
    CHECK(e.auth.login("8000000001", "wrong") == nullptr);
    CHECK(e.auth.login("0000000000", "pass") == nullptr);
    CHECK(e.auth.updateProfile(1, "New Name", "8111111111", e.err));
    CHECK(e.auth.login("8111111111", "pass") != nullptr && e.auth.login("8000000001", "pass") == nullptr);
    CHECK(AuthenticationManager::sanitize("a|b") == "a b");
}

static void testDispatch() {         // Dhairya's module
    std::cout << "[dispatch]\n";
    Env e; e.addDrivers();
    auto best = e.dispatcher.findBestDriver(3);                    // pickup Clock Tower
    CHECK(best && best->driverId == 1 && best->distanceKm == 0);   // D One is at the pickup
    e.auth.getDriver(1)->setAvailable(false);
    best = e.dispatcher.findBestDriver(3);
    CHECK(best && best->driverId == 3 && best->distanceKm == 1);   // Railway Stn is 1 km away
    CHECK(DriverDispatcher::compositeScore(0, 5.0) > DriverDispatcher::compositeScore(1, 5.0));
    CHECK(DriverDispatcher::compositeScore(2, 5.0) > DriverDispatcher::compositeScore(2, 3.0));
    e.auth.getDriver(2)->setAvailable(false); e.auth.getDriver(3)->setAvailable(false);
    CHECK(!e.dispatcher.findBestDriver(3).has_value());            // nobody free
    // tie-break: equal distance + rating -> smaller id first
    Env t; t.auth.registerDriver("X", "9000000011", "pass", "c", 3, t.err);
    t.auth.registerDriver("Y", "9000000012", "pass", "c", 3, t.err);
    CHECK(t.dispatcher.findBestDriver(3)->driverId == 1);
    auto top = t.dispatcher.topCandidates(3, 5);
    CHECK(top.size() == 2 && top[0].driverId == 1 && top[1].driverId == 2);
    // a higher-rated driver can beat a slightly closer one
    Env r; r.auth.registerDriver("Near", "9000000021", "pass", "c", 4, r.err);   // 1 km, 4.0
    r.auth.registerDriver("Top", "9000000022", "pass", "c", 3, r.err);           // 0 km, 4.0
    for (int i = 0; i < 3; ++i) r.auth.getDriver(1)->addRating(5.0);             // Near -> 5.0 (first rating replaces default)
    CHECK(r.dispatcher.findBestDriver(3)->driverId == 2);                        // 0 km still wins: proximity weighs more
}

static void testBooking() {          // Tanvi (create/cancel/fare) + Nishtha (complete/rate/queue/history)
    std::cout << "[booking]\n";
    FareCalculator f;
    CHECK(f.calculate(1) == 60.0 && f.calculate(8) == 136.0 && f.calculate(8, 1.25) == 170.0);

    Env e; e.addDrivers(); int c = e.addCustomer();
    CHECK(e.bm.quoteFare(8) == 136.0);                             // website fare preview, no surge
    Booking invalidBooking;
    CHECK(!e.bm.bookRide(9999, 1, 10, invalidBooking, e.err));       // unknown customer
    CHECK(!e.bm.bookRide(1, 1, 10, invalidBooking, e.err));          // driver is not a customer
    Booking b1, b2, b3, b4, tmp;
    CHECK(e.bm.bookRide(c, 1, 10, b1, e.err) && b1.status == "CONFIRMED" && b1.distanceKm == 8);
    CHECK(!e.auth.getDriver(b1.driverId)->isAvailable());
    CHECK(!e.bm.bookRide(c, 1, 1, tmp, e.err));                    // same pickup/drop
    CHECK(!e.bm.bookRide(c, 1, 99, tmp, e.err));                   // unknown location
    CHECK(e.bm.bookRide(c, 3, 9, b2, e.err) && b2.status == "CONFIRMED");
    CHECK(e.bm.bookRide(c, 4, 2, b3, e.err) && b3.status == "CONFIRMED");
    CHECK(e.bm.bookRide(c, 5, 9, b4, e.err) && b4.status == "PENDING" && e.bm.pendingCount() == 1);
    CHECK(e.bm.activeBookingOfDriver(b1.driverId) != nullptr);
    CHECK(e.bm.completeRide(b1.driverId, tmp, e.err) && tmp.status == "COMPLETED");
    CHECK(e.auth.getDriver(b1.driverId)->getLocation() == 10);     // driver moved to drop node
    CHECK(e.bm.pendingCount() == 0);                               // waiting request auto-assigned
    CHECK(e.bm.all().back().status == "CONFIRMED");
    CHECK(!e.bm.completeRide(999, tmp, e.err));                    // no such active ride
    CHECK(e.bm.rateRide(b1.id, c, 5, e.err));
    CHECK(!e.bm.rateRide(b1.id, c, 4, e.err));                     // already rated
    CHECK(!e.bm.rateRide(b2.id, c, 4, e.err));                     // not completed
    CHECK(!e.bm.rateRide(b1.id, c + 1, 4, e.err));                 // someone else's booking
    CHECK(e.auth.getDriver(b1.driverId)->getRating() == 5.0);
    int cancelled = e.bm.cancelLastBooking(c, e.err);              // stack: newest first
    CHECK(cancelled == b4.id);
    CHECK(e.bm.cancelBooking(b2.id, c, e.err));
    CHECK(!e.bm.cancelBooking(b1.id, c, e.err));                   // COMPLETED can't be cancelled
    CHECK(e.bm.historyOf(c).size() == 4);
}

static void testPersistence() {      // Dhairya (file handling)
    std::cout << "[storage]\n";
    const char* uf = "test_users.tmp"; const char* bf = "test_bookings.tmp";
    FileStorageManager fs(uf, bf);
    int cid; Booking b;
    {
        Env e; e.addDrivers(); cid = e.addCustomer(); e.auth.ensureDefaultAdmin();
        e.bm.bookRide(cid, 1, 10, b, e.err);
        CHECK(fs.saveUsers(e.auth) && fs.saveBookings(e.bm));
    }
    {
        Env e; fs.loadUsers(e.auth); fs.loadBookings(e.bm);
        CHECK(e.auth.allUsers().size() == 5);
        CHECK(e.auth.login("8000000001", "pass") != nullptr);       // hash survives a reload
        CHECK(e.auth.login("9999999999", "admin123") != nullptr);
        CHECK(e.bm.all().size() == 1 && e.bm.all()[0].status == "CONFIRMED");
        CHECK(e.bm.all()[0].fare == 136.0);
        CHECK(!e.auth.getDriver(b.driverId)->isAvailable());        // availability flag persisted
        Booking nb;
        CHECK(e.bm.bookRide(cid, 3, 9, nb, e.err) && nb.id == 2);   // ids continue after reload
    }
    FileStorageManager missing("no_such_users.tmp", "no_such_bookings.tmp");
    Env e2; missing.loadUsers(e2.auth); missing.loadBookings(e2.bm);  // first run: must not crash
    CHECK(e2.auth.allUsers().empty());
    std::remove(uf); std::remove(bf);
}

int main() {
    testGraph(); testAuth(); testDispatch(); testBooking(); testPersistence();
    std::cout << "\nPassed: " << passed << "  Failed: " << failed << "\n";
    return failed == 0 ? 0 : 1;
}
