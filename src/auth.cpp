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
bool AuthenticationManager::validate(const std::string& name, const std::string& phone,
                                     const std::string& password, std::string& err) const {
    if (name.empty()) { err = "Name cannot be empty."; return false; }
    if (phone.size() != 10 || !std::all_of(phone.begin(), phone.end(),
                                           [](unsigned char c) { return std::isdigit(c); })) {
        err = "Phone number must be exactly 10 digits.";
        return false;
    }
    if (phoneIndex.count(phone)) { err = "This phone number is already registered."; return false; }
    if (password.size() < 4) { err = "Password must have at least 4 characters."; return false; }
    return true;
}
std::shared_ptr<Customer> AuthenticationManager::registerCustomer(
    const std::string& name, const std::string& phone, const std::string& password,
    int homeLocation, std::string& err) {
    if (!validate(name, phone, password, err)) return nullptr;
    auto c = std::make_shared<Customer>(nextId++, sanitize(name), phone, hashPassword(password),
                                        homeLocation);
    users[c->getId()] = c;
    phoneIndex[phone] = c->getId();
    return c;
}
std::shared_ptr<Driver> AuthenticationManager::registerDriver(
    const std::string& name, const std::string& phone, const std::string& password,
    const std::string& vehicle, int location, std::string& err) {
    if (!validate(name, phone, password, err)) return nullptr;
    auto d = std::make_shared<Driver>(nextId++, sanitize(name), phone, hashPassword(password),
                                      sanitize(vehicle), location);
    users[d->getId()] = d;
    phoneIndex[phone] = d->getId();
    return d;
}


std::shared_ptr<Admin> AuthenticationManager::registerAdmin(
    const std::string& name, const std::string& phone, const std::string& password,
    std::string& err) {
    if (!validate(name, phone, password, err)) return nullptr;
    auto a = std::make_shared<Admin>(nextId++, sanitize(name), phone, hashPassword(password));
    users[a->getId()] = a;
    phoneIndex[phone] = a->getId();
    return a;
}


std::shared_ptr<User> AuthenticationManager::login(const std::string& phone,
                                                   const std::string& password) const {
    auto it = phoneIndex.find(phone);
    if (it == phoneIndex.end()) return nullptr;
    auto u = users.at(it->second);
    return (u->getPasswordHash() == hashPassword(password)) ? u : nullptr;
}
bool AuthenticationManager::updateProfile(int userId, const std::string& newName,
                                          const std::string& newPhone, std::string& err) {
    auto it = users.find(userId);
    if (it == users.end()) { err = "User not found."; return false; }
    auto& u = it->second;
    if (!newName.empty()) u->setName(sanitize(newName));
    if (!newPhone.empty() && newPhone != u->getPhone()) {
        bool digits = std::all_of(newPhone.begin(), newPhone.end(),
                                  [](unsigned char c) { return std::isdigit(c); });
        if (newPhone.size() != 10 || !digits) { err = "Phone must be exactly 10 digits."; return false; }
        if (phoneIndex.count(newPhone)) { err = "Phone number already in use."; return false; }
        phoneIndex.erase(u->getPhone());
        u->setPhone(newPhone);
        phoneIndex[newPhone] = userId;
    }
    return true;
}
std::shared_ptr<User> AuthenticationManager::getUser(int id) const {
    auto it = users.find(id);
    return it == users.end() ? nullptr : it->second;
}


std::shared_ptr<Driver> AuthenticationManager::getDriver(int id) const {
    return std::dynamic_pointer_cast<Driver>(getUser(id));
}

std::vector<std::shared_ptr<User>> AuthenticationManager::allUsers() const {
    std::vector<std::shared_ptr<User>> v;
    for (const auto& kv : users) v.push_back(kv.second);
    std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) { return a->getId() < b->getId(); });
    return v;
}
std::vector<std::shared_ptr<Driver>> AuthenticationManager::allDrivers() const {
    std::vector<std::shared_ptr<Driver>> v;
    for (const auto& u : allUsers())
        if (auto d = std::dynamic_pointer_cast<Driver>(u)) v.push_back(d);
    return v;
}

void AuthenticationManager::addLoadedUser(std::shared_ptr<User> u) {
    nextId = std::max(nextId, u->getId() + 1);
    phoneIndex[u->getPhone()] = u->getId();
    users[u->getId()] = std::move(u);
}

void AuthenticationManager::ensureDefaultAdmin() {
    for (const auto& kv : users)
        if (kv.second->getRole() == Role::ADMIN) return;
    std::string err;
    registerAdmin("System Admin", "9999999999", "admin123", err);
}

bool AuthenticationManager::hasDrivers() const {
    for (const auto& kv : users)
        if (kv.second->getRole() == Role::DRIVER) return true;
    return false;
}
