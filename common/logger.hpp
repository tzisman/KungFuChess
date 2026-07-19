#pragma once

#include <string>

namespace kfc::common {

// A tiny thread-safe logger: writes timestamped "[tag] message" lines to stdout.
// Used by the server and client composition roots to trace what happens on the
// wire. It is a plain utility and holds no game or networking types.
class Logger {
public:
    explicit Logger(std::string tag);

    void info(const std::string& message) const;

private:
    std::string tag_;
};

}
