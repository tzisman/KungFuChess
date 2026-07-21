#pragma once

#include <memory>
#include <string>

#include "server/user_store.hpp"

namespace kfc::server {

// The one implementation of UserStore, backed by SQLite. sqlite3.h is confined
// to the .cpp via the pimpl below, so including this header pulls in none of
// it — the same isolation as WebsocketppServer for net::ServerTransport.
class SqliteUserStore : public UserStore {
public:
    explicit SqliteUserStore(const std::string& path);  // ":memory:" in tests
    ~SqliteUserStore() override;

    bool registerUser(const std::string& username, const std::string& password) override;
    std::optional<UserRecord> authenticate(const std::string& username, const std::string& password) override;
    void updateRating(const std::string& username, int rating) override;
    std::optional<int> ratingOf(const std::string& username) const override;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace kfc::server
