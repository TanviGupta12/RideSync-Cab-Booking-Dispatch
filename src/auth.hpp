
#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class Role { CUSTOMER, DRIVER, ADMIN };

std::string roleToString(Role r);
Role roleFromString(const std::string& s); 


class User {
protected:
    int id;
    std::string name;
    std::string phone;        
    std::string passwordHash;  
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

class Customer : public User {
    int homeLocation;  

public:
    Customer(int id, std::string name, std::string phone, std::string passwordHash, int homeLocation);
    int getHomeLocation() const { return homeLocation; }
    void setHomeLocation(int loc) { homeLocation = loc; }
    void showProfile() const override;
};


class Driver : public User {
    int location; 
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


class Admin : public User {
public:
    Admin(int id, std::string name, std::string phone, std::string passwordHash);
    void showProfile() const override;
};


class AuthenticationManager {
    std::unordered_map<int, std::shared_ptr<User>> users; 
    std::unordered_map<std::string, int> phoneIndex;     
    int nextId = 1;

    bool validate(const std::string& name, const std::string& phone,
                  const std::string& password, std::string& err) const;

public:
    static std::string hashPassword(const std::string& plain);
    static std::string sanitize(std::string s);  

    std::shared_ptr<Customer> registerCustomer(const std::string& name, const std::string& phone,
                                               const std::string& password, int homeLocation,
                                               std::string& err);
    std::shared_ptr<Driver> registerDriver(const std::string& name, const std::string& phone,
                                           const std::string& password, const std::string& vehicle,
                                           int location, std::string& err);
    std::shared_ptr<Admin> registerAdmin(const std::string& name, const std::string& phone,
                                         const std::string& password, std::string& err);

    std::shared_ptr<User> login(const std::string& phone, const std::string& password) const;
    bool updateProfile(int userId, const std::string& newName, const std::string& newPhone,
                       std::string& err);

    std::shared_ptr<User> getUser(int id) const;
    std::shared_ptr<Driver> getDriver(int id) const;
    std::vector<std::shared_ptr<User>> allUsers() const;   
    std::vector<std::shared_ptr<Driver>> allDrivers() const; 

    void addLoadedUser(std::shared_ptr<User> u);  
    void ensureDefaultAdmin();
    bool hasDrivers() const;
};
