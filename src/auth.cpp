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
