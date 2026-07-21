#include <doctest/doctest.h>

#include "server/password_hash.hpp"

using kfc::server::generateSalt;
using kfc::server::hashPassword;
using kfc::server::verifyPassword;

TEST_CASE("the same password and salt always hash the same") {
    const std::string salt = generateSalt();
    CHECK(hashPassword("hunter2", salt) == hashPassword("hunter2", salt));
}

TEST_CASE("different passwords with the same salt hash differently") {
    const std::string salt = generateSalt();
    CHECK(hashPassword("hunter2", salt) != hashPassword("hunter3", salt));
}

TEST_CASE("verifyPassword accepts the right password and rejects a wrong one") {
    const std::string salt = generateSalt();
    const std::string hash = hashPassword("hunter2", salt);
    CHECK(verifyPassword("hunter2", salt, hash));
    CHECK_FALSE(verifyPassword("wrong", salt, hash));
}

TEST_CASE("two calls to generateSalt produce different salts") {
    CHECK(generateSalt() != generateSalt());
}
