#include "auth.hpp"

#include <algorithm>
#include <cctype>
#include <functional>
#include <iomanip>
#include <iostream>

std::string roleToString(Role r) {
    switch (r) {
        case Role::CUSTOMER: return "CUSTOMER";
        case Role::DRIVER:   return "DRIVER";
        case Role::ADMIN:    return "ADMIN";
    }
    return "CUSTOMER";
}

Role roleFromString(const std::string& s) {
    if (s == "DRIVER") return Role::DRIVER;
    if (s == "ADMIN") return Role::ADMIN;
    return Role::CUSTOMER;
}
//User
User::User(int id, std::string name, std::string phone, std::string passwordHash, Role role)
    : id(id), name(std::move(name)), phone(std::move(phone)),
      passwordHash(std::move(passwordHash)), role(role) {}

void User::showProfile() const {
    std::cout << "  ID    : " << id << "\n  Name  : " << name << "\n  Phone : " << phone
              << "\n  Role  : " << roleToString(role) << "\n";
}
//Customer
Customer::Customer(int id, std::string name, std::string phone, std::string passwordHash,
                   int homeLocation)
    : User(id, std::move(name), std::move(phone), std::move(passwordHash), Role::CUSTOMER),
      homeLocation(homeLocation) {}

void Customer::showProfile() const {
    User::showProfile();
    std::cout << "  Home location id : " << homeLocation << "\n";
}
//Driver
Driver::Driver(int id, std::string name, std::string phone, std::string passwordHash,
               std::string vehicle, int location, double rating, int ratingCount, bool available)
    : User(id, std::move(name), std::move(phone), std::move(passwordHash), Role::DRIVER),
      location(location), rating(rating), ratingCount(ratingCount), available(available),
      vehicle(std::move(vehicle)) {}

void Driver::addRating(double stars) {
    rating = (rating * ratingCount + stars) / (ratingCount + 1);
    ++ratingCount;
}

void Driver::showProfile() const {
    User::showProfile();
    std::cout << "  Vehicle  : " << vehicle << "\n  Location : node " << location
              << "\n  Rating   : " << std::fixed << std::setprecision(2) << rating << " ("
              << ratingCount << " rides rated)\n  Status   : "
              << (available ? "AVAILABLE" : "OFFLINE / ON TRIP") << "\n";
}
//Admin
Admin::Admin(int id, std::string name, std::string phone, std::string passwordHash)
    : User(id, std::move(name), std::move(phone), std::move(passwordHash), Role::ADMIN) {}

void Admin::showProfile() const { User::showProfile(); }
std::string AuthenticationManager::hashPassword(const std::string& plain) {
    return std::to_string(std::hash<std::string>{}("ridesync$" + plain));
}
std::string AuthenticationManager::sanitize(std::string s) {
    std::replace(s.begin(), s.end(), '|', ' ');
    std::replace(s.begin(), s.end(), '\n', ' ');
    std::replace(s.begin(), s.end(), '\r', ' ');
    return s;
}
