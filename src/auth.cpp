#include "auth.hpp"
#include<iostream>
#include <functional>
#include <iomanip>
#include <sstream>
#include <cctype>
#include <utility>

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

User::User(int id, std::string name, std::string phone,
           std::string passwordHash, Role role)
    : id(id), name(std::move(name)), phone(std::move(phone)),
      passwordHash(std::move(passwordHash)), role(role) {}

void User::showProfile() const {
    std::cout << "ID: " << id << "\nName: " << name
              << "\nPhone: " << phone
              << "\nRole: " << roleToString(role) << '\n';
}

Customer::Customer(int id, std::string name, std::string phone,
                   std::string passwordHash, int homeLocation)
    : User(id, std::move(name), std::move(phone),
           std::move(passwordHash), Role::CUSTOMER),
      homeLocation(homeLocation) {}

void Customer::showProfile() const {
    User::showProfile();
    std::cout << "Home location: " << homeLocation << '\n';
}

Driver::Driver(int id, std::string name, std::string phone,
               std::string passwordHash, std::string vehicle,
               int location, double rating, int ratingCount,
               bool available)
    : User(id, std::move(name), std::move(phone),
           std::move(passwordHash), Role::DRIVER),
      location(location), rating(rating), ratingCount(ratingCount),
      available(available), vehicle(std::move(vehicle)) {}

void Driver::addRating(double stars) {
    if (stars < 1.0 || stars > 5.0) return;

    rating = (rating * ratingCount + stars) / (ratingCount + 1);
    ++ratingCount;
}

void Driver::showProfile() const {
    User::showProfile();
    std::cout << "Vehicle: " << vehicle
              << "\nLocation: " << location
              << "\nRating: " << rating
              << "\nAvailable: " << (available ? "Yes" : "No")
              << '\n';
}

Admin::Admin(int id, std::string name, std::string phone,
             std::string passwordHash)
    : User(id, std::move(name), std::move(phone),
           std::move(passwordHash), Role::ADMIN) {}

void Admin::showProfile() const {
    User::showProfile();
}

std::string AuthenticationManager::hashPassword(
    const std::string& plain) {
    // Educational hash only; not suitable for production authentication.
    std::size_t value = std::hash<std::string>{}(plain);
    std::ostringstream out;
    out << std::hex << value;
    return out.str();
}

std::string AuthenticationManager::sanitize(std::string s) {
    for (char& ch : s) {
        if (ch == '|' || ch == '\n' || ch == '\r') {
            ch = ' ';
        }
    }
    return s;
}

bool AuthenticationManager::validate(
    const std::string& name, const std::string& phone,
    const std::string& password, std::string& err) const {

    if (name.empty()) {
        err = "Name cannot be empty.";
        return false;
    }

    if (phone.size() != 10) {
        err = "Phone number must contain 10 digits.";
        return false;
    }

    for (unsigned char ch : phone) {
        if (!std::isdigit(ch)) {
            err = "Phone number must contain only digits.";
            return false;
        }
    }

    if (password.size() < 4) {
        err = "Password must contain at least 4 characters.";
        return false;
    }

    if (phoneIndex.find(phone) != phoneIndex.end()) {
        err = "This phone number is already registered.";
        return false;
    }

    return true;
}

std::shared_ptr<Customer> AuthenticationManager::registerCustomer(
    const std::string& name, const std::string& phone,
    const std::string& password, int homeLocation, std::string& err) {

    if (!validate(name, phone, password, err)) return nullptr;

    int id = nextId++;
    auto user = std::make_shared<Customer>(
        id, sanitize(name), phone, hashPassword(password), homeLocation);

    users[id] = user;
    phoneIndex[phone] = id;
    return user;
}

std::shared_ptr<Driver> AuthenticationManager::registerDriver(
    const std::string& name, const std::string& phone,
    const std::string& password, const std::string& vehicle,
    int location, std::string& err) {

    if (!validate(name, phone, password, err)) return nullptr;

    int id = nextId++;
    auto user = std::make_shared<Driver>(
        id, sanitize(name), phone, hashPassword(password),
        sanitize(vehicle), location);

    users[id] = user;
    phoneIndex[phone] = id;
    return user;
}

std::shared_ptr<Admin> AuthenticationManager::registerAdmin(
    const std::string& name, const std::string& phone,
    const std::string& password, std::string& err) {

    if (!validate(name, phone, password, err)) return nullptr;

    int id = nextId++;
    auto user = std::make_shared<Admin>(
        id, sanitize(name), phone, hashPassword(password));

    users[id] = user;
    phoneIndex[phone] = id;
    return user;
}

std::shared_ptr<User> AuthenticationManager::login(
    const std::string& phone, const std::string& password) const {

    auto it = phoneIndex.find(phone);
    if (it == phoneIndex.end()) return nullptr;

    auto userIt = users.find(it->second);
    if (userIt == users.end()) return nullptr;

    if (userIt->second->getPasswordHash() != hashPassword(password)) {
        return nullptr;
    }

    return userIt->second;
}

bool AuthenticationManager::updateProfile(
    int userId, const std::string& newName,
    const std::string& newPhone, std::string& err) {

    auto it = users.find(userId);
    if (it == users.end()) {
        err = "User not found.";
        return false;
    }

    if (newName.empty()) {
        err = "Name cannot be empty.";
        return false;
    }

    if (newPhone.size() != 10) {
        err = "Phone number must contain 10 digits.";
        return false;
    }

    for (unsigned char ch : newPhone) {
        if (!std::isdigit(ch)) {
            err = "Phone number must contain only digits.";
            return false;
        }
    }

    auto phoneIt = phoneIndex.find(newPhone);
    if (phoneIt != phoneIndex.end() && phoneIt->second != userId) {
        err = "This phone number is already registered.";
        return false;
    }

    phoneIndex.erase(it->second->getPhone());
    it->second->setName(sanitize(newName));
    it->second->setPhone(newPhone);
    phoneIndex[newPhone] = userId;

    return true;
}

std::shared_ptr<User> AuthenticationManager::getUser(int id) const {
    auto it = users.find(id);
    return it == users.end() ? nullptr : it->second;
}

std::shared_ptr<Driver> AuthenticationManager::getDriver(int id) const {
    return std::dynamic_pointer_cast<Driver>(getUser(id));
}

std::vector<std::shared_ptr<User>>
AuthenticationManager::allUsers() const {
    std::vector<std::shared_ptr<User>> result;
    for (const auto& entry : users) {
        result.push_back(entry.second);
    }
    return result;
}

std::vector<std::shared_ptr<Driver>>
AuthenticationManager::allDrivers() const {
    std::vector<std::shared_ptr<Driver>> result;
    for (const auto& entry : users) {
        auto driver = std::dynamic_pointer_cast<Driver>(entry.second);
        if (driver) result.push_back(driver);
    }
    return result;
}

void AuthenticationManager::addLoadedUser(std::shared_ptr<User> user) {
    if (!user) return;

    users[user->getId()] = user;
    phoneIndex[user->getPhone()] = user->getId();

    if (user->getId() >= nextId) {
        nextId = user->getId() + 1;
    }
}

void AuthenticationManager::ensureDefaultAdmin() {
    for (const auto& entry : users) {
        if (entry.second->getRole() == Role::ADMIN) return;
    }

    std::string err;
    registerAdmin("Administrator", "9999999999", "admin123", err);
}

bool AuthenticationManager::hasDrivers() const {
    for (const auto& entry : users) {
        if (entry.second->getRole() == Role::DRIVER) return true;
    }
    return false;
}
