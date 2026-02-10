#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <librtc/data_channel.hpp>
#include <librtc/utils/event.hpp>
#include <librtc/utils/expected.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace librtc {

enum class SignalingState {
  Stable,
  HaveLocalOffer,
  HaveLocalPrAnswer,
  HaveRemoteOffer,
  HaveRemotePrAnswer,
  Closed
};

enum class IceConnectionState { New, Checking, Connected, Completed, Failed, Disconnected, Closed };

enum class IceGatheringState { New, Gathering, Complete };

struct IceServer {
  std::vector<std::string> urls;
  std::string username;
  std::string credential;
};

struct PeerConnectionConfig {
  std::vector<IceServer> ice_servers;
};

struct SessionDescription {
  std::string type;
  std::string sdp;
};

struct IceCandidate {
  std::string candidate;
  std::string sdp_mid;
  int sdp_mline_index;
};

class PeerConnection {
 public:
  template <typename T>
  using Task = boost::asio::awaitable<Expected<T>>;

  virtual ~PeerConnection() = default;

  static Expected<std::shared_ptr<PeerConnection>> Create(
      std::optional<boost::asio::any_io_executor> executor = std::nullopt,
      const PeerConnectionConfig& config = {});

  // Event handlers
  EVENT(ice_candidate, const IceCandidate&)
  EVENT(data_channel, std::shared_ptr<DataChannel>)
  EVENT(ice_connection_state_change, IceConnectionState)
  EVENT(signaling_state_change, SignalingState)

  // Actions
  virtual Task<SessionDescription> create_offer() = 0;
  virtual Task<SessionDescription> create_answer() = 0;
  virtual Task<void> set_local_description(const SessionDescription& sdp) = 0;
  virtual Task<void> set_remote_description(const SessionDescription& sdp) = 0;

  virtual std::optional<SessionDescription> local_description() const = 0;
  virtual std::optional<SessionDescription> remote_description() const = 0;

  virtual Expected<void> add_ice_candidate(const IceCandidate& candidate) = 0;
  virtual Expected<std::shared_ptr<DataChannel>> create_data_channel(
      const std::string& label, const DataChannelConfig& config = {}) = 0;

  virtual SignalingState signaling_state() const = 0;
  virtual IceConnectionState ice_connection_state() const = 0;
  virtual IceGatheringState ice_gathering_state() const = 0;

  virtual void close() = 0;
};

}  // namespace librtc
