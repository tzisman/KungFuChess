#pragma once

#include <optional>
#include <string>

namespace kfc::server {

inline constexpr int kDefaultRating = 1000;

struct UserRecord {
    std::string username;
    int rating;
};

// The identity/rating seam between server logic and whatever backs accounts.
// Kept as an interface so server logic can be unit-tested against an
// in-memory fake without linking SQLite at all.
class UserStore {
public:
    virtual ~UserStore() = default;

    virtual bool registerUser(const std::string& username, const std::string& password) = 0;
    virtual std::optional<UserRecord> authenticate(const std::string& username, const std::string& password) = 0;
    virtual void updateRating(const std::string& username, int rating) = 0;
    virtual std::optional<int> ratingOf(const std::string& username) const = 0;
};

}  // namespace kfc::server
