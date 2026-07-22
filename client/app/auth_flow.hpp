#pragma once

#include "app/network_session.hpp"
#include "common/logger.hpp"
#include "net/websocketpp_transport.hpp"

namespace kfc::app {

// Drives the terminal register/login prompts until the session reports a
// successful login. Registering succeeds into an automatic login attempt
// with the same credentials; either one being rejected loops back to ask
// again. A dropped connection during either wait has nothing left to retry
// against, so it ends the process.
void loginInteractively(net::WebsocketppClient& transport, common::Logger& log,
                        NetworkSession& session);

}
