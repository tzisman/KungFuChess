#include "net/websocketpp_transport.hpp"

#include <map>
#include <mutex>
#include <optional>
#include <utility>
#include <vector>

#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/server.hpp>

namespace kfc::net {
namespace {
using WsServer = websocketpp::server<websocketpp::config::asio>;
using WsClient = websocketpp::client<websocketpp::config::asio_client>;
using Hdl = websocketpp::connection_hdl;
}  // namespace

struct WebsocketppServer::Impl {
    WsServer endpoint;

    // The maps are read from the io thread (handlers) and the game thread
    // (send/broadcast), so every touch is guarded.
    std::mutex mutex;
    std::map<ConnectionId, Hdl> byId;
    std::map<Hdl, ConnectionId, std::owner_less<Hdl>> idByHdl;
    ConnectionId nextId = 1;

    ServerTransport::OpenHandler onOpen;
    ServerTransport::MessageHandler onMessage;
    ServerTransport::CloseHandler onClose;

    std::optional<ConnectionId> idOf(Hdl hdl) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = idByHdl.find(hdl);
        if (it == idByHdl.end()) return std::nullopt;
        return it->second;
    }
    std::optional<Hdl> hdlOf(ConnectionId id) {
        std::lock_guard<std::mutex> lock(mutex);
        auto it = byId.find(id);
        if (it == byId.end()) return std::nullopt;
        return it->second;
    }
    std::vector<Hdl> allHandles() {
        std::lock_guard<std::mutex> lock(mutex);
        std::vector<Hdl> handles;
        handles.reserve(byId.size());
        for (const auto& [id, hdl] : byId) handles.push_back(hdl);
        return handles;
    }
};

WebsocketppServer::WebsocketppServer() : impl_(std::make_unique<Impl>()) {}
WebsocketppServer::~WebsocketppServer() = default;

void WebsocketppServer::onOpen(OpenHandler handler) {
    impl_->onOpen = std::move(handler);
}
void WebsocketppServer::onMessage(MessageHandler handler) {
    impl_->onMessage = std::move(handler);
}
void WebsocketppServer::onClose(CloseHandler handler) {
    impl_->onClose = std::move(handler);
}

void WebsocketppServer::send(ConnectionId connection, const std::string& message) {
    std::optional<Hdl> hdl = impl_->hdlOf(connection);
    if (!hdl) return;
    websocketpp::lib::error_code ec;
    impl_->endpoint.send(*hdl, message, websocketpp::frame::opcode::text, ec);
}

void WebsocketppServer::broadcast(const std::string& message) {
    for (const Hdl& hdl : impl_->allHandles()) {
        websocketpp::lib::error_code ec;
        impl_->endpoint.send(hdl, message, websocketpp::frame::opcode::text, ec);
    }
}

void WebsocketppServer::listen(std::uint16_t port) {
    impl_->endpoint.clear_access_channels(websocketpp::log::alevel::all);
    impl_->endpoint.init_asio();
    impl_->endpoint.set_reuse_addr(true);

    impl_->endpoint.set_open_handler([this](Hdl hdl) {
        ConnectionId id;
        {
            std::lock_guard<std::mutex> lock(impl_->mutex);
            id = impl_->nextId++;
            impl_->byId[id] = hdl;
            impl_->idByHdl[hdl] = id;
        }
        if (impl_->onOpen) impl_->onOpen(id);
    });

    impl_->endpoint.set_close_handler([this](Hdl hdl) {
        std::optional<ConnectionId> id = impl_->idOf(hdl);
        if (id && impl_->onClose) impl_->onClose(*id);
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (id) impl_->byId.erase(*id);
        impl_->idByHdl.erase(hdl);
    });

    impl_->endpoint.set_message_handler([this](Hdl hdl, WsServer::message_ptr msg) {
        std::optional<ConnectionId> id = impl_->idOf(hdl);
        if (id && impl_->onMessage) impl_->onMessage(*id, msg->get_payload());
    });

    impl_->endpoint.listen(port);
    impl_->endpoint.start_accept();
}

void WebsocketppServer::run() { impl_->endpoint.run(); }

void WebsocketppServer::stop() {
    websocketpp::lib::error_code ec;
    impl_->endpoint.stop_listening(ec);
    for (const Hdl& hdl : impl_->allHandles()) {
        websocketpp::lib::error_code close_ec;
        impl_->endpoint.close(hdl, websocketpp::close::status::going_away, "",
                              close_ec);
    }
    impl_->endpoint.stop();
}

// --- client ---

struct WebsocketppClient::Impl {
    WsClient endpoint;
    Hdl connection;

    ClientTransport::OpenHandler onOpen;
    ClientTransport::MessageHandler onMessage;
    ClientTransport::CloseHandler onClose;
};

WebsocketppClient::WebsocketppClient() : impl_(std::make_unique<Impl>()) {}
WebsocketppClient::~WebsocketppClient() = default;

void WebsocketppClient::onOpen(OpenHandler handler) {
    impl_->onOpen = std::move(handler);
}
void WebsocketppClient::onMessage(MessageHandler handler) {
    impl_->onMessage = std::move(handler);
}
void WebsocketppClient::onClose(CloseHandler handler) {
    impl_->onClose = std::move(handler);
}

void WebsocketppClient::send(const std::string& message) {
    websocketpp::lib::error_code ec;
    impl_->endpoint.send(impl_->connection, message,
                         websocketpp::frame::opcode::text, ec);
}

void WebsocketppClient::connect(const std::string& uri) {
    impl_->endpoint.clear_access_channels(websocketpp::log::alevel::all);
    impl_->endpoint.init_asio();

    impl_->endpoint.set_open_handler([this](Hdl) {
        if (impl_->onOpen) impl_->onOpen();
    });
    impl_->endpoint.set_message_handler([this](Hdl, WsClient::message_ptr msg) {
        if (impl_->onMessage) impl_->onMessage(msg->get_payload());
    });
    // A failed handshake and a clean close both mean "no connection" to us.
    impl_->endpoint.set_close_handler([this](Hdl) {
        if (impl_->onClose) impl_->onClose();
    });
    impl_->endpoint.set_fail_handler([this](Hdl) {
        if (impl_->onClose) impl_->onClose();
    });

    websocketpp::lib::error_code ec;
    WsClient::connection_ptr connection = impl_->endpoint.get_connection(uri, ec);
    if (ec) {
        if (impl_->onClose) impl_->onClose();
        return;
    }
    impl_->connection = connection->get_handle();
    impl_->endpoint.connect(connection);
}

void WebsocketppClient::run() { impl_->endpoint.run(); }

void WebsocketppClient::stop() { impl_->endpoint.stop(); }

}
