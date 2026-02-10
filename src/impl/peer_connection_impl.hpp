#pragma once

#include <api/peer_connection_interface.h>
#include <rtc_base/thread.h>

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/awaitable.hpp>
#include <librtc/peer_connection.hpp>
#include <mutex>
#include <optional>

namespace librtc {

class PeerConnectionObserverProxy;

class PeerConnectionImpl : public PeerConnection,
                           public std::enable_shared_from_this<PeerConnectionImpl> {
 public:
  static Expected<std::shared_ptr<PeerConnectionImpl>> Create(
      std::optional<boost::asio::any_io_executor> executor, const PeerConnectionConfig& config);

  explicit PeerConnectionImpl(std::optional<boost::asio::any_io_executor> executor);

  // Internal initialization
  void set_threads_and_factory(
      std::unique_ptr<webrtc::Thread> network_thread, std::unique_ptr<webrtc::Thread> worker_thread,
      std::unique_ptr<webrtc::Thread> signaling_thread,
      webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory);
  void set_pc(webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc);

  ~PeerConnectionImpl() override;

  // PeerConnection Interface Implementation
  Event<const IceCandidate&>& on_ice_candidate() override {
    return ice_candidate_event;
  }
  Event<std::shared_ptr<DataChannel>>& on_data_channel() override {
    return data_channel_event;
  }
  Event<IceConnectionState>& on_ice_connection_state_change() override {
    return ice_connection_state_event;
  }
  Event<SignalingState>& on_signaling_state_change() override {
    return signaling_state_event;
  }

  Task<SessionDescription> create_offer() override;
  Task<SessionDescription> create_answer() override;
  Task<void> set_local_description(const SessionDescription& sdp) override;
  Task<void> set_remote_description(const SessionDescription& sdp) override;

  std::optional<SessionDescription> local_description() const override;
  std::optional<SessionDescription> remote_description() const override;

  Expected<void> add_ice_candidate(const IceCandidate& candidate) override;
  Expected<std::shared_ptr<DataChannel>> create_data_channel(
      const std::string& label, const DataChannelConfig& config) override;

  SignalingState signaling_state() const override;
  IceConnectionState ice_connection_state() const override;
  IceGatheringState ice_gathering_state() const override;

  void close() override;

  // Handlers for proxy
  void handle_signaling_change(SignalingState new_state);
  void handle_ice_connection_change(IceConnectionState new_state);
  void handle_ice_gathering_change(IceGatheringState new_state);
  void handle_ice_candidate(const IceCandidate& ice);
  void handle_data_channel(std::shared_ptr<DataChannel> channel);

  // Internal event sources
  EventSource<const IceCandidate&> ice_candidate_event;
  EventSource<std::shared_ptr<DataChannel>> data_channel_event;
  EventSource<IceConnectionState> ice_connection_state_event;
  EventSource<SignalingState> signaling_state_event;

 private:
  // Destruction order matters! Destroyed in reverse order of declaration.
  std::unique_ptr<webrtc::Thread> network_thread_;
  std::unique_ptr<webrtc::Thread> worker_thread_;
  std::unique_ptr<webrtc::Thread> signaling_thread_;
  webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> pc_factory_;
  webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc_;

  std::unique_ptr<PeerConnectionObserverProxy> observer_proxy_;
  std::optional<boost::asio::any_io_executor> executor_;
  mutable std::mutex mutex_;

  SignalingState cached_signaling_state_ = SignalingState::Stable;
  IceConnectionState cached_ice_connection_state_ = IceConnectionState::New;
  IceGatheringState cached_ice_gathering_state_ = IceGatheringState::New;
};

}  // namespace librtc
