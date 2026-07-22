#include "app/auth_flow.hpp"

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <thread>

#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

namespace kfc::app {
namespace {

constexpr int kAuthPollMs = 20;

struct Credentials {
    std::string username;
    std::string password;
};

bool isPlainAscii(const std::string& text) {
    for (unsigned char c : text) {
        if (c < 0x20 || c > 0x7E) return false;
    }
    return true;
}

std::string askNonEmptyAscii(const std::string& prompt) {
    while (true) {
        std::cout << prompt;
        std::string value;
        std::getline(std::cin, value);
        if (!value.empty() && isPlainAscii(value)) return value;
        std::cout << "Please use plain English letters and digits.\n";
    }
}

Credentials askForCredentials() {
    std::string username = askNonEmptyAscii("Username (English letters/digits only): ");
    std::cout << "Password: ";
    std::string password;
    std::getline(std::cin, password);
    return {username, password};
}

bool askWantsToRegister() {
    while (true) {
        std::cout << "Register a new account or log in to an existing one? (r/l): ";
        std::string choice;
        std::getline(std::cin, choice);
        if (choice == "r" || choice == "R") return true;
        if (choice == "l" || choice == "L") return false;
    }
}

// Blocks until the session confirms registration, rejects it (username
// taken), or the connection drops. A dropped connection has nothing left to
// retry against, so it ends the process; a rejection returns false so the
// caller can ask for different credentials.
bool awaitRegistration(NetworkSession& session, common::Logger& log) {
    while (true) {
        if (session.registered()) {
            log.info("registration succeeded");
            return true;
        }
        if (std::optional<std::string> rejection = session.authRejection()) {
            std::cout << "Registration rejected: " << *rejection << "\n";
            log.info("registration rejected: " + *rejection);
            return false;
        }
        if (session.closed()) {
            std::cerr << "Connection closed before registering\n";
            std::exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kAuthPollMs));
    }
}

// Blocks until the session confirms login (returning the account's rating),
// rejects it (bad credentials), or the connection drops. A bad login returns
// nullopt so the caller can ask again; a dropped connection has nothing left
// to retry against, so that ends the process instead.
std::optional<int> awaitLogin(NetworkSession& session, common::Logger& log) {
    while (true) {
        if (std::optional<int> rating = session.loginRating()) {
            log.info("login succeeded, rating " + std::to_string(*rating));
            return rating;
        }
        if (std::optional<std::string> rejection = session.authRejection()) {
            std::cout << "Login rejected: " << *rejection << "\n";
            log.info("login rejected: " + *rejection);
            return std::nullopt;
        }
        if (session.closed()) {
            std::cerr << "Connection closed before logging in\n";
            std::exit(1);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(kAuthPollMs));
    }
}

}  // namespace

void loginInteractively(net::WebsocketppClient& transport, common::Logger& log,
                        NetworkSession& session) {
    while (true) {
        bool wantsRegister = askWantsToRegister();
        Credentials creds = askForCredentials();

        if (wantsRegister) {
            session.resetAuth();
            log.info("registering as " + creds.username);
            transport.send(protocol::encode(protocol::Message{
                protocol::RegisterRequest{creds.username, creds.password}}));
            if (!awaitRegistration(session, log)) continue;
            std::cout << "Registered. Logging in...\n";
        }

        session.resetAuth();
        log.info("logging in as " + creds.username);
        transport.send(protocol::encode(
            protocol::Message{protocol::LoginRequest{creds.username, creds.password}}));
        if (awaitLogin(session, log)) return;
    }
}

}
