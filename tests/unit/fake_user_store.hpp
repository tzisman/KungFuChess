#pragma once

#include <map>
#include <optional>
#include <string>

#include "server/user_store.hpp"

namespace kfc::test {

// An in-memory UserStore for tests that never touches SQLite. Password
// checking is a plain string comparison — good enough for driving ServerApp
// through login/register flows without pulling in server::hashPassword.
class FakeUserStore : public server::UserStore {
public:
    bool registerUser(const std::string& username, const std::string& password) override {
        if (users_.count(username)) return false;
        users_[username] = {password, server::kDefaultRating};
        return true;
    }

    std::optional<server::UserRecord> authenticate(const std::string& username,
                                                     const std::string& password) override {
        auto it = users_.find(username);
        if (it == users_.end() || it->second.password != password) return std::nullopt;
        return server::UserRecord{username, it->second.rating};
    }

    void updateRating(const std::string& username, int rating) override {
        auto it = users_.find(username);
        if (it != users_.end()) it->second.rating = rating;
    }

    std::optional<int> ratingOf(const std::string& username) const override {
        auto it = users_.find(username);
        if (it == users_.end()) return std::nullopt;
        return it->second.rating;
    }

private:
    struct StoredUser {
        std::string password;
        int rating;
    };
    std::map<std::string, StoredUser> users_;
};

}  // namespace kfc::test
