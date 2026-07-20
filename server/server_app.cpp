#include "server/server_app.hpp"

#include <string>
#include <variant>

#include "protocol/json_codec.hpp"
#include "protocol/messages.hpp"

namespace kfc::server {

ServerApp::ServerApp(net::ServerTransport& transport, common::Logger& log,
                     CommandQueue& commands, PlayerNames& names)
    : transport_(transport), log_(log), commands_(commands), names_(names) {
    transport_.onOpen([this](net::ConnectionId id) { onOpen(id); });
    transport_.onMessage([this](net::ConnectionId id, const std::string& text) {
        onMessage(id, text);
    });
    transport_.onClose([this](net::ConnectionId id) { onClose(id); });
}

void ServerApp::onOpen(net::ConnectionId id) {
    log_.info("connection " + std::to_string(id) + " opened");
}

void ServerApp::onMessage(net::ConnectionId id, const std::string& text) {
    std::optional<protocol::Message> message = protocol::decode(text);
    if (!message) {
        log_.info("connection " + std::to_string(id) + " sent malformed input");
        return;
    }
    if (const auto* join = std::get_if<protocol::JoinRequest>(&*message)) {
        handleJoin(id, join->name);
        return;
    }
    if (const auto* move = std::get_if<protocol::MoveIntent>(&*message)) {
        handleMove(id, *move);
        return;
    }
    if (const auto* jump = std::get_if<protocol::JumpIntent>(&*message)) {
        handleJump(id, *jump);
        return;
    }
    log_.info("connection " + std::to_string(id) + " sent an unexpected message");
}

void ServerApp::handleJoin(net::ConnectionId id, const std::string& name) {
    if (sessions_.count(id)) {
        log_.info("connection " + std::to_string(id) + " already joined");
        return;
    }
    std::optional<model::Color> color = freeColor();
    if (!color) {
        log_.info(name + " (connection " + std::to_string(id) +
                  ") rejected: game full");
        transport_.send(id, protocol::encode(protocol::Rejected{"full"}));
        return;
    }
    sessions_.emplace(id, Session{id, *color, name});
    names_.set(*color, name);
    log_.info(name + " joined as " + model::nameOf(*color) + " (connection " +
              std::to_string(id) + ")");
    transport_.send(id, protocol::encode(protocol::Assigned{*color}));
}

void ServerApp::handleMove(net::ConnectionId id,
                           const protocol::MoveIntent& intent) {
    std::optional<model::Color> color = colorOf(id);
    if (!color) return;
    commands_.push(MoveCommand{*color, intent.from, intent.to});
}

void ServerApp::handleJump(net::ConnectionId id,
                           const protocol::JumpIntent& intent) {
    std::optional<model::Color> color = colorOf(id);
    if (!color) return;
    commands_.push(JumpCommand{*color, intent.cell});
}

std::optional<model::Color> ServerApp::colorOf(net::ConnectionId id) const {
    auto it = sessions_.find(id);
    if (it == sessions_.end()) return std::nullopt;
    return it->second.color;
}

void ServerApp::onClose(net::ConnectionId id) {
    auto it = sessions_.find(id);
    if (it == sessions_.end()) {
        log_.info("connection " + std::to_string(id) + " closed");
        return;
    }
    log_.info(it->second.name + " (" + model::nameOf(it->second.color) +
              ") left; colour freed");
    sessions_.erase(it);
}

std::optional<model::Color> ServerApp::freeColor() const {
    for (model::Color color : model::kAllColors) {
        bool taken = false;
        for (const auto& [id, session] : sessions_) {
            if (session.color == color) {
                taken = true;
                break;
            }
        }
        if (!taken) return color;
    }
    return std::nullopt;
}

}
