// Compile-only check that the vendored asio + websocketpp headers build with
// this toolchain. It holds no logic yet; kfc_net gains its real transport
// sources in a later step. If the vendored versions are incompatible with the
// compiler, the break surfaces here instead of deep inside the first feature.
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/server.hpp>

namespace kfc::net {

using ServerEndpoint = websocketpp::server<websocketpp::config::asio>;
using ClientEndpoint = websocketpp::client<websocketpp::config::asio_client>;

}  // namespace kfc::net
