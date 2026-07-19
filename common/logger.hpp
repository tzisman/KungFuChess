#pragma once

#include <iosfwd>
#include <string>

namespace kfc::common {

// A tiny thread-safe logger: writes timestamped "[tag] message" lines to an
// output stream (stdout by default). Used by the server and client composition
// roots to trace what happens on the wire, and pointed at a string stream by
// tests. It is a plain utility and holds no game or networking types.
class Logger {
public:
    explicit Logger(std::string tag);
    Logger(std::string tag, std::ostream& out);

    void info(const std::string& message) const;

private:
    std::string tag_;
    std::ostream& out_;
};

}
