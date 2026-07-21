#include "server/password_hash.hpp"

#include <cstdint>
#include <iomanip>
#include <random>
#include <sstream>

namespace kfc::server {

namespace {

constexpr int kHexWidth = 16;

std::string toHex(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(kHexWidth) << value;
    return stream.str();
}

}  // namespace

std::string generateSalt() {
    std::random_device device;
    std::mt19937_64 generator(device());
    std::uniform_int_distribution<std::uint64_t> distribution;
    return toHex(distribution(generator));
}

std::string hashPassword(const std::string& password, const std::string& salt) {
    const std::size_t digest = std::hash<std::string>{}(salt + password);
    return toHex(static_cast<std::uint64_t>(digest));
}

bool verifyPassword(const std::string& password, const std::string& salt, const std::string& expectedHash) {
    return hashPassword(password, salt) == expectedHash;
}

}  // namespace kfc::server
