// =============================================================================
//  main.cpp  --  RideSync : Console CLI
//  Owners : Member 1 (Tanvi Gupta - Team Lead) & Member 3 (Nishtha Tyagi)
//  Build  : g++ -std=c++17 -Wall -Wextra -o ridesync main.cpp auth.cpp graph.cpp dispatch.cpp system.cpp
// =============================================================================
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include "auth.hpp"
#include "dispatch.hpp"
#include "graph.hpp"
#include "system.hpp"

using namespace std;

struct InputClosed {};  // thrown when stdin ends (Ctrl+D / piped input finished)

static string readLine(const string& prompt) {
    cout << prompt;
    string s;
    if (!getline(cin, s)) throw InputClosed{};
    size_t a = s.find_first_not_of(" \t\r\n"), b = s.find_last_not_of(" \t\r\n");
    return a == string::npos ? "" : s.substr(a, b - a + 1);
}

static int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try { size_t pos; int v = stoi(s, &pos); if (pos == s.size()) return v; } catch (...) {}
        cout << "  Please enter a whole number.\n";
    }
}

// Everything the CLI needs, wired together in dependency order.
struct App {
    CityGraph city;
    RoutePlanner planner;
    AuthenticationManager auth;
    DriverDispatcher dispatcher;
    BookingManager bookings;
    FileStorageManager storage;

    App() : city(), planner(city), auth(), dispatcher(auth, planner),
            bookings(auth, planner, dispatcher), storage() {}

    void save() {
        if (!storage.saveUsers(auth) || !storage.saveBookings(bookings))
            cout << "  [warning] could not write data files.\n";
    }
};

static void showBooking(const App& app, const Booking& b) {
    cout << "  Booking #" << b.id << " [" << b.status << "]  " << b.timestamp << "\n"
         << "    " << app.city.nameOf(b.pickup) << " -> " << app.city.nameOf(b.drop) << "  ("
         << b.distanceKm << " km)\n    Fare: Rs. " << fixed << setprecision(2) << b.fare;
    if (b.driverId) {
        auto d = app.auth.getDriver(b.driverId);
        cout << "   Driver: " << (d ? d->getName() + " (" + d->getVehicle() + ")" : "?");
    }
    if (b.rating > 0) cout << "   Rated: " << b.rating;
    cout << "\n";
}

static void showLocations(const App& app) {
    cout << "\n  Locations:\n";
    for (int id : app.city.nodeIds()) cout << "   " << setw(2) << id << ". " << app.city.nameOf(id) << "\n";
}

static void profileMenu(App& app, const shared_ptr<User>& u) {
    u->showProfile();
    string ans = readLine("  Update profile? (y/n): ");
    if (ans != "y" && ans != "Y") return;
    string name = readLine("  New name (blank = keep): ");
    string phone = readLine("  New phone (blank = keep): ");
    string err;
    if (!app.auth.updateProfile(u->getId(), name, phone, err)) { cout << "  " << err << "\n"; return; }
    if (auto c = dynamic_pointer_cast<Customer>(u)) {
        string loc = readLine("  New home location id (blank = keep): ");
        if (!loc.empty()) {
            try { int v = stoi(loc); if (app.city.hasNode(v)) c->setHomeLocation(v); else cout << "  Unknown location.\n"; }
            catch (...) { cout << "  Invalid number.\n"; }
        }
    }
    app.save();
    cout << "  Profile updated.\n";
}

// ------------------------------- Customer -----------------------------------
static void customerMenu(App& app, const shared_ptr<Customer>& me) {
    while (true) {
        cout << "\n--- Customer: " << me->getName() << " ---\n"
             << " 1. Book a ride\n 2. Booking history\n 3. Cancel last booking\n"
             << " 4. Rate a completed ride\n 5. Profile\n 0. Logout\n";
        int ch = readInt("Choice: ");
        string err;
        if (ch == 0) return;
        if (ch == 1) {
            showLocations(app);
            int p = readInt("  Pickup location id: ");
            int d = readInt("  Drop location id  : ");
            Booking b;
            if (!app.bookings.bookRide(me->getId(), p, d, b, err)) { cout << "  " << err << "\n"; continue; }
            cout << "\n  Route: " << app.planner.describe(app.planner.shortestPath(p, d)) << "\n";
            showBooking(app, b);
            if (b.status == "CONFIRMED") cout << "  Driver is " << b.pickupDistKm << " km from your pickup.\n";
            else cout << "  No driver free right now - you are in the waiting queue.\n";
            app.save();
        } else if (ch == 2) {
            auto h = app.bookings.historyOf(me->getId());
            if (h.empty()) cout << "  No bookings yet.\n";
            for (const auto& b : h) showBooking(app, b);
        } else if (ch == 3) {
            int id = app.bookings.cancelLastBooking(me->getId(), err);
            if (id < 0) cout << "  " << err << "\n";
            else { cout << "  Booking #" << id << " cancelled.\n"; app.save(); }
        } else if (ch == 4) {
            int id = readInt("  Booking id to rate: ");
            int stars = readInt("  Stars (1-5): ");
            if (app.bookings.rateRide(id, me->getId(), stars, err)) { cout << "  Thanks for rating!\n"; app.save(); }
            else cout << "  " << err << "\n";
        } else if (ch == 5) profileMenu(app, me);
        else cout << "  Invalid choice.\n";
    }
}

// -------------------------------- Driver ------------------------------------
static void driverMenu(App& app, const shared_ptr<Driver>& me) {
    while (true) {
        cout << "\n--- Driver: " << me->getName() << " ---\n"
             << " 1. View profile / update\n 2. Toggle availability\n 3. Current ride\n"
             << " 4. Complete current ride\n 0. Logout\n";
        int ch = readInt("Choice: ");
        string err;
        if (ch == 0) return;
        if (ch == 1) profileMenu(app, me);
        else if (ch == 2) {
            if (app.bookings.activeBookingOfDriver(me->getId())) { cout << "  Finish your current ride first.\n"; continue; }
            me->setAvailable(!me->isAvailable());
            cout << "  You are now " << (me->isAvailable() ? "AVAILABLE" : "OFFLINE") << ".\n";
            if (me->isAvailable() && app.bookings.processPending() > 0) cout << "  A waiting request was assigned.\n";
            app.save();
        } else if (ch == 3) {
            const Booking* b = app.bookings.activeBookingOfDriver(me->getId());
            if (b) showBooking(app, *b); else cout << "  No active ride.\n";
        } else if (ch == 4) {
            Booking b;
            if (app.bookings.completeRide(me->getId(), b, err)) { cout << "  Ride completed. Fare collected: Rs. " << fixed << setprecision(2) << b.fare << "\n"; app.save(); }
            else cout << "  " << err << "\n";
        } else cout << "  Invalid choice.\n";
    }
}

// -------------------------------- Admin -------------------------------------
static void adminMenu(App& app, const shared_ptr<Admin>& me) {
    while (true) {
        cout << "\n--- Admin: " << me->getName() << " ---\n"
             << " 1. All users\n 2. All bookings\n 3. City map\n 4. Process waiting queue\n"
             << " 5. Dispatcher ranking for a pickup\n 6. Shortest route query\n 0. Logout\n";
        int ch = readInt("Choice: ");
        if (ch == 0) return;
        if (ch == 1) {
            for (const auto& u : app.auth.allUsers()) { u->showProfile(); cout << "  ----\n"; }
        } else if (ch == 2) {
            if (app.bookings.all().empty()) cout << "  No bookings.\n";
            for (const auto& b : app.bookings.all()) showBooking(app, b);
        } else if (ch == 3) {
            cout << "\n  " << app.city.nodeCount() << " locations, " << app.city.edgeCount() << " roads\n";
            app.city.print();
        } else if (ch == 4) {
            int n = app.bookings.processPending();
            cout << "  Assigned " << n << " waiting request(s); " << app.bookings.pendingCount() << " still waiting.\n";
            app.save();
        } else if (ch == 5) {
            showLocations(app);
            int p = readInt("  Pickup location id: ");
            auto list = app.dispatcher.topCandidates(p, 5);
            if (list.empty()) cout << "  No available driver can reach that point.\n";
            int rank = 1;
            for (const auto& c : list) {
                auto d = app.auth.getDriver(c.driverId);
                cout << "  #" << rank++ << " " << (d ? d->getName() : "?") << "  dist=" << c.distanceKm
                     << " km  rating=" << fixed << setprecision(2) << c.rating << "  score=" << setprecision(3) << c.score << "\n";
            }
        } else if (ch == 6) {
            showLocations(app);
            int s = readInt("  From id: "), t = readInt("  To id  : ");
            auto r = app.planner.shortestPath(s, t);
            if (!r.found) cout << "  No route.\n";
            else cout << "  " << app.planner.describe(r) << "  (" << r.distanceKm << " km)\n";
        } else cout << "  Invalid choice.\n";
    }
}

// ------------------------------ Auth screens --------------------------------
static void registerFlow(App& app) {
    cout << "\n  Register as: 1. Customer  2. Driver\n";
    int kind = readInt("  Choice: ");
    if (kind != 1 && kind != 2) { cout << "  Invalid choice.\n"; return; }
    string name = readLine("  Name: "), phone = readLine("  Phone (10 digits): "), pass = readLine("  Password (min 4): ");
    showLocations(app);
    int loc = readInt(kind == 1 ? "  Home location id: " : "  Current location id: ");
    if (!app.city.hasNode(loc)) { cout << "  Unknown location.\n"; return; }
    string err;
    bool ok;
    if (kind == 1) ok = app.auth.registerCustomer(name, phone, pass, loc, err) != nullptr;
    else {
        string veh = readLine("  Vehicle (e.g. Swift UK07AB1234): ");
        ok = app.auth.registerDriver(name, phone, pass, veh, loc, err) != nullptr;
    }
    if (ok) { cout << "  Registration successful. You can log in now.\n"; app.save(); }
    else cout << "  " << err << "\n";
}

static void loginFlow(App& app) {
    string phone = readLine("  Phone: "), pass = readLine("  Password: ");
    auto u = app.auth.login(phone, pass);
    if (!u) { cout << "  Invalid phone or password.\n"; return; }
    if (auto c = dynamic_pointer_cast<Customer>(u)) customerMenu(app, c);
    else if (auto d = dynamic_pointer_cast<Driver>(u)) driverMenu(app, d);
    else if (auto a = dynamic_pointer_cast<Admin>(u)) adminMenu(app, a);
}

static void seedDemoDrivers(App& app) {
    struct Seed { const char* n; const char* ph; const char* v; int loc; };
    const Seed seeds[] = {{"Rakesh Negi", "9000000001", "Swift UK07CA1001", 3},
                          {"Anil Rawat", "9000000002", "WagonR UK07CB2002", 1},
                          {"Sunil Thapa", "9000000003", "Dzire UK07CC3003", 4},
                          {"Mohit Bisht", "9000000004", "Ertiga UK07CD4004", 10}};
    string err;
    for (const auto& s : seeds) app.auth.registerDriver(s.n, s.ph, "driver123", s.v, s.loc, err);
}

int main() {
    App app;
    app.city.loadDefaultDehradun();
    app.storage.loadUsers(app.auth);
    app.storage.loadBookings(app.bookings);
    app.auth.ensureDefaultAdmin();
    if (!app.auth.hasDrivers()) seedDemoDrivers(app);
    app.save();

    cout << "=====================================================\n"
         << "  RideSync - Smart Cab Booking & Dispatch Simulation\n"
         << "  Team The Four Bytes | DSCPP-III-2026-T151\n"
         << "=====================================================\n"
         << "  Demo logins -> admin: 9999999999 / admin123, driver: 9000000001 / driver123\n";
    try {
        while (true) {
            cout << "\n--- Main Menu ---\n 1. Login\n 2. Register\n 0. Exit\n";
            int ch = readInt("Choice: ");
            if (ch == 0) break;
            if (ch == 1) loginFlow(app);
            else if (ch == 2) registerFlow(app);
            else cout << "  Invalid choice.\n";
        }
    } catch (const InputClosed&) {
        cout << "\n  [input closed]\n";
    }
    app.save();
    cout << "  Data saved. Goodbye!\n";
    return 0;
}
