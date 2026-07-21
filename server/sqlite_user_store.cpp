#include "server/sqlite_user_store.hpp"

#include <sqlite3.h>

#include "server/password_hash.hpp"

namespace kfc::server {

namespace {

constexpr const char* kCreateTableSql =
    "CREATE TABLE IF NOT EXISTS users ("
    "username TEXT PRIMARY KEY, "
    "salt TEXT NOT NULL, "
    "password_hash TEXT NOT NULL, "
    "rating INTEGER NOT NULL)";
constexpr const char* kInsertUserSql =
    "INSERT INTO users (username, salt, password_hash, rating) VALUES (?, ?, ?, ?)";
constexpr const char* kSelectUserSql = "SELECT salt, password_hash, rating FROM users WHERE username = ?";
constexpr const char* kUpdateRatingSql = "UPDATE users SET rating = ? WHERE username = ?";

struct StoredUser {
    std::string salt;
    std::string passwordHash;
    int rating;
};

std::optional<StoredUser> findUser(sqlite3* db, const std::string& username) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(db, kSelectUserSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<StoredUser> found;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = StoredUser{
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)),
            sqlite3_column_int(stmt, 2),
        };
    }
    sqlite3_finalize(stmt);
    return found;
}

}  // namespace

struct SqliteUserStore::Impl {
    sqlite3* db = nullptr;

    ~Impl() {
        if (db) sqlite3_close(db);
    }
};

SqliteUserStore::SqliteUserStore(const std::string& path) : impl_(std::make_unique<Impl>()) {
    sqlite3_open(path.c_str(), &impl_->db);
    sqlite3_exec(impl_->db, kCreateTableSql, nullptr, nullptr, nullptr);
}

SqliteUserStore::~SqliteUserStore() = default;

bool SqliteUserStore::registerUser(const std::string& username, const std::string& password) {
    if (findUser(impl_->db, username)) return false;

    const std::string salt = generateSalt();
    const std::string hash = hashPassword(password, salt);

    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(impl_->db, kInsertUserSql, -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, salt.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, hash.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, kDefaultRating);
    const bool inserted = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return inserted;
}

std::optional<UserRecord> SqliteUserStore::authenticate(const std::string& username, const std::string& password) {
    std::optional<StoredUser> stored = findUser(impl_->db, username);
    if (!stored) return std::nullopt;
    if (!verifyPassword(password, stored->salt, stored->passwordHash)) return std::nullopt;
    return UserRecord{username, stored->rating};
}

void SqliteUserStore::updateRating(const std::string& username, int rating) {
    sqlite3_stmt* stmt = nullptr;
    sqlite3_prepare_v2(impl_->db, kUpdateRatingSql, -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, rating);
    sqlite3_bind_text(stmt, 2, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

std::optional<int> SqliteUserStore::ratingOf(const std::string& username) const {
    std::optional<StoredUser> stored = findUser(impl_->db, username);
    if (!stored) return std::nullopt;
    return stored->rating;
}

}  // namespace kfc::server
