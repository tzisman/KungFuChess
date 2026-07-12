#pragma once

#include <stdexcept>

namespace kfc {

// Base for every KungFuChess domain error; each layer derives its own.
class Error : public std::logic_error {
public:
    using std::logic_error::logic_error;
};

}
