#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class Role { CUSTOMER, DRIVER, ADMIN };

std::string roleToString(Role r);
Role roleFromString(const std::string& s);

//User class
class User {
protected:
    int id;
    std::string name;
    std::string phone;         // phone number doubles as the login id
    std::string passwordHash;  // never store the plain password
    Role role;

public:
    User(int id, std::string name, std::string phone, std::string passwordHash, Role role);
    virtual ~User() = default;

    int getId() const { return id; }
    const std::string& getName() const { return name; }
    const std::string& getPhone() const { return phone; }
    const std::string& getPasswordHash() const { return passwordHash; }
    Role getRole() const { return role; }

    void setName(const std::string& n) { name = n; }
    void setPhone(const std::string& p) { phone = p; }

    virtual void showProfile() const;
};

//Customer class
class Customer : public User {
    int homeLocation;  // node id on the city graph

public:
    Customer(int id, std::string name, std::string phone, std::string passwordHash, int homeLocation);
    int getHomeLocation() const { return homeLocation; }
    void setHomeLocation(int loc) { homeLocation = loc; }
    void showProfile() const override;
};

//Driver class
class Driver : public User {
    int location;  // current node id
    double rating;
    int ratingCount;
    bool available;
    std::string vehicle;

public:
    Driver(int id, std::string name, std::string phone, std::string passwordHash,
           std::string vehicle, int location, double rating = 4.0, int ratingCount = 0,
           bool available = true);

    int getLocation() const { return location; }
    double getRating() const { return rating; }
    int getRatingCount() const { return ratingCount; }
    bool isAvailable() const { return available; }
    const std::string& getVehicle() const { return vehicle; }

    void setLocation(int loc) { location = loc; }
    void setAvailable(bool a) { available = a; }
    void addRating(double stars); 
    void showProfile() const override;
};
