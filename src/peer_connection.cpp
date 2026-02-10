#include <librtc/peer_connection.hpp>

#include "impl/peer_connection_impl.hpp"

namespace librtc {

Expected<std::shared_ptr<PeerConnection>> PeerConnection::Create(
    std::optional<boost::asio::any_io_executor> executor, const PeerConnectionConfig& config) {
  auto impl_result = PeerConnectionImpl::Create(std::move(executor), config);

  if (!impl_result) {
    return Err(impl_result.error());
  }

  // Explicitly return as the correct Result type
  return std::shared_ptr<PeerConnection>(impl_result.value());
}

}  // namespace librtc
