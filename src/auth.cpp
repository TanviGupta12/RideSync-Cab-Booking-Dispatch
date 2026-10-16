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

User::User(int id, std::string name, std::string phone, std::string passwordHash, Role role)
    : id(id), name(std::move(name)), phone(std::move(phone)),
      passwordHash(std::move(passwordHash)), role(role) {}

void User::showProfile() const {
    std::cout << "  ID    : " << id << "\n  Name  : " << name << "\n  Phone : " << phone
              << "\n  Role  : " << roleToString(role) << "\n";
}

Customer::Customer(int id, std::string name, std::string phone, std::string passwordHash,
                   int homeLocation)
    : User(id, std::move(name), std::move(phone), std::move(passwordHash), Role::CUSTOMER),
      homeLocation(homeLocation) {}

void Customer::showProfile() const {
    User::showProfile();
    std::cout << "  Home location id : " << homeLocation << "\n";
}

