
#include "system.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

static std::string nowString() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", std::localtime(&t));
    return buf;
}


double FareCalculator::calculate(int distanceKm, double surge) const {
    double fare = std::max(minFare, baseFare + perKm * distanceKm) * surge;
    return std::round(fare * 100.0) / 100.0;
}


Booking* BookingManager::find(int bookingId) {
    for (auto& b : bookings)
        if (b.id == bookingId) return &b;
    return nullptr;
}

bool BookingManager::assign(Booking& b) {
    auto best = dispatcher.findBestDriver(b.pickup);
    if (!best) return false;
    auto driver = auth.getDriver(best->driverId);
    if (!driver) return false;
    driver->setAvailable(false);
    b.driverId = driver->getId();
    b.pickupDistKm = best->distanceKm;
    b.status = "CONFIRMED";
    return true;
}

double BookingManager::quoteFare(int distanceKm) const {
    double surge = pending.size() >= 3 ? 1.25 : 1.0; 
    return fareCalc.calculate(distanceKm, surge);
}

bool BookingManager::bookRide(int customerId, int pickup, int drop, Booking& out,
                              std::string& err) {
    auto customer = std::dynamic_pointer_cast<Customer>(auth.getUser(customerId));
    if (!customer) { err = "Unknown customer account."; return false; }
    if (pickup == drop) { err = "Pickup and drop locations must be different."; return false; }
    RouteResult route = planner.shortestPath(pickup, drop);
    if (!route.found) { err = "Invalid location or no road route between the two points."; return false; }

    Booking b;
    b.id = nextId++;
    b.customerId = customerId;
    b.pickup = pickup;
    b.drop = drop;
    b.distanceKm = route.distanceKm;
    b.fare = quoteFare(route.distanceKm);
    b.timestamp = nowString();

    if (!assign(b)) {         
        b.status = "PENDING";
        pending.push(b.id);
    }
    bookings.push_back(b);
    recent[customerId].push(b.id);
    out = b;
    return true;
}

bool BookingManager::completeRide(int driverId, Booking& out, std::string& err) {
    for (auto& b : bookings) {
        if (b.driverId == driverId && b.status == "CONFIRMED") {
            b.status = "COMPLETED";
            if (auto d = auth.getDriver(driverId)) {
                d->setLocation(b.drop); 
                d->setAvailable(true);
            }
            out = b;
            processPending();
            return true;
        }
    }
    err = "You have no active ride.";
    return false;
}

bool BookingManager::cancelBooking(int bookingId, int customerId, std::string& err) {
    Booking* b = find(bookingId);
    if (!b || b->customerId != customerId) { err = "Booking not found."; return false; }
    if (b->status != "PENDING" && b->status != "CONFIRMED") {
        err = "Only PENDING or CONFIRMED bookings can be cancelled.";
        return false;
    }
    if (b->status == "CONFIRMED")
        if (auto d = auth.getDriver(b->driverId)) d->setAvailable(true);
    b->status = "CANCELLED";   
    processPending();
    return true;
}

int BookingManager::cancelLastBooking(int customerId, std::string& err) {
    auto it = recent.find(customerId);
    if (it != recent.end()) {
        auto& st = it->second;
        while (!st.empty()) {
            int id = st.top();
            st.pop();
            Booking* b = find(id);
            if (b && (b->status == "PENDING" || b->status == "CONFIRMED")) {
                std::string ignored;
                if (cancelBooking(id, customerId, ignored)) return id;
            }
        }
    }
    err = "No active booking to cancel.";
    return -1;
}

bool BookingManager::rateRide(int bookingId, int customerId, double stars, std::string& err) {
    Booking* b = find(bookingId);
    if (!b || b->customerId != customerId) { err = "Booking not found."; return false; }
    if (b->status != "COMPLETED") { err = "Only completed rides can be rated."; return false; }
    if (b->rating > 0.0) { err = "This ride is already rated."; return false; }
    if (stars < 1.0 || stars > 5.0) { err = "Rating must be between 1 and 5."; return false; }
    b->rating = stars;
    if (auto d = auth.getDriver(b->driverId)) d->addRating(stars);
    return true;
}

int BookingManager::processPending() {
    int assigned = 0;
    while (!pending.empty()) {
        Booking* b = find(pending.front());
        if (!b || b->status != "PENDING") { pending.pop(); continue; }  
        if (!assign(*b)) break; 
        pending.pop();
        ++assigned;
    }
    return assigned;
}

std::vector<Booking> BookingManager::historyOf(int customerId) const {
    std::vector<Booking> h;
    for (const auto& b : bookings)
        if (b.customerId == customerId) h.push_back(b);
    return h;
}

const Booking* BookingManager::activeBookingOfDriver(int driverId) const {
    for (const auto& b : bookings)
        if (b.driverId == driverId && b.status == "CONFIRMED") return &b;
    return nullptr;
}

void BookingManager::load(std::vector<Booking> loaded) {
    bookings = std::move(loaded);
    pending = {};
    recent.clear();
    nextId = 1;
    for (const auto& b : bookings) {
        nextId = std::max(nextId, b.id + 1);
        recent[b.customerId].push(b.id);
        if (b.status == "PENDING") pending.push(b.id);
    }
}



static std::vector<std::string> split(const std::string& line, char delim = '|') {
    std::vector<std::string> parts;
    std::string item;
    std::istringstream ss(line);
    while (std::getline(ss, item, delim)) parts.push_back(item);
    if (!line.empty() && line.back() == delim) parts.push_back("");
    return parts;
}

bool FileStorageManager::saveUsers(const AuthenticationManager& auth) const {
    std::ofstream out(usersFile);
    if (!out) return false;
    out << std::fixed << std::setprecision(2);
    for (const auto& u : auth.allUsers()) {
        int loc = 0; double rating = 0; int cnt = 0; int avail = 0; std::string veh = "-";
        if (auto c = std::dynamic_pointer_cast<Customer>(u)) loc = c->getHomeLocation();
        if (auto d = std::dynamic_pointer_cast<Driver>(u)) {
            loc = d->getLocation(); rating = d->getRating(); cnt = d->getRatingCount();
            avail = d->isAvailable() ? 1 : 0; veh = d->getVehicle();
        }
        out << roleToString(u->getRole()) << '|' << u->getId() << '|' << u->getName() << '|'
            << u->getPhone() << '|' << u->getPasswordHash() << '|' << loc << '|' << rating << '|'
            << cnt << '|' << avail << '|' << veh << '\n';
    }
    return true;
}

bool FileStorageManager::saveBookings(const BookingManager& bm) const {
    std::ofstream out(bookingsFile);
    if (!out) return false;
    out << std::fixed << std::setprecision(2);
    for (const auto& b : bm.all())
        out << b.id << '|' << b.customerId << '|' << b.driverId << '|' << b.pickup << '|'
            << b.drop << '|' << b.distanceKm << '|' << b.pickupDistKm << '|' << b.fare << '|'
            << b.status << '|' << b.rating << '|' << b.timestamp << '\n';
    return true;
}

void FileStorageManager::loadUsers(AuthenticationManager& auth) const {
    std::ifstream in(usersFile);
    if (!in) return; 
    std::string line;
    while (std::getline(in, line)) {
        auto f = split(line);
        if (f.size() < 10) continue;  
        try {
            Role role = roleFromString(f[0]);
            int id = std::stoi(f[1]);
            int loc = std::stoi(f[5]);
            if (role == Role::CUSTOMER)
                auth.addLoadedUser(std::make_shared<Customer>(id, f[2], f[3], f[4], loc));
            else if (role == Role::DRIVER)
                auth.addLoadedUser(std::make_shared<Driver>(id, f[2], f[3], f[4], f[9], loc,
                                                            std::stod(f[6]), std::stoi(f[7]),
                                                            f[8] == "1"));
            else
                auth.addLoadedUser(std::make_shared<Admin>(id, f[2], f[3], f[4]));
        } catch (...) {  }
    }
}

void FileStorageManager::loadBookings(BookingManager& bm) const {
    std::ifstream in(bookingsFile);
    if (!in) return;
    std::vector<Booking> loaded;
    std::string line;
    while (std::getline(in, line)) {
        auto f = split(line);
        if (f.size() < 11) continue;
        try {
            Booking b;
            b.id = std::stoi(f[0]);       b.customerId = std::stoi(f[1]);
            b.driverId = std::stoi(f[2]); b.pickup = std::stoi(f[3]);
            b.drop = std::stoi(f[4]);     b.distanceKm = std::stoi(f[5]);
            b.pickupDistKm = std::stoi(f[6]); b.fare = std::stod(f[7]);
            b.status = f[8];              b.rating = std::stod(f[9]);
            b.timestamp = f[10];
            loaded.push_back(b);
        } catch (...) {  }
    }
    bm.load(std::move(loaded));
}
