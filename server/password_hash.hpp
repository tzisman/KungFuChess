#pragma once

#include <string>

namespace kfc::server {

// Not cryptographic: std::hash offers no resistance to reversal or collision
// attacks. Accepted scope limit for this project, not production-grade security.
std::string generateSalt();
std::string hashPassword(const std::string& password, const std::string& salt);
bool verifyPassword(const std::string& password, const std::string& salt, const std::string& expectedHash);

}  // namespace kfc::server
