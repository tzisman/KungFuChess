#include "common/logger.hpp"

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <utility>

namespace kfc::common {
namespace {

std::mutex& outputMutex() {
    static std::mutex mutex;
    return mutex;
}

std::string nowText() {
    std::time_t now = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now());
    std::ostringstream out;
    out << std::put_time(std::localtime(&now), "%H:%M:%S");
    return out.str();
}

}  // namespace

Logger::Logger(std::string tag) : tag_(std::move(tag)) {}

void Logger::info(const std::string& message) const {
    std::lock_guard<std::mutex> lock(outputMutex());
    std::cout << nowText() << " [" << tag_ << "] " << message << std::endl;
}

}
