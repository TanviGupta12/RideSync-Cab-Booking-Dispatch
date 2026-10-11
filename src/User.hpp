#ifndef USER_HPP
#define USER_HPP

#include <string>
#include <iostream>
enum class UserRole { CUSTOMER, DRIVER, ADMIN };

class User {
protected:
    std::string userId;
    std::string name;
    std::string phone;
    std::string password;
    UserRole role;

public:
    User() = default;
    User(std::string id, std::string n, std::string p, std::string pass, UserRole r)
        : userId(id), name(n), phone(p), password(pass), role(r) {}

    virtual ~User() = default;

    std::string getId() const { return userId; }
    std::string getName() const { return name; }
    std::string getPhone() const { return phone; }
    UserRole getRole() const { return role; }

    bool authenticate(const std::string& inputPass) const {
        return password == inputPass;
    }
};

