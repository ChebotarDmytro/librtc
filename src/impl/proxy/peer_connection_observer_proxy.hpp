#pragma once

#include <api/peer_connection_interface.h>

#include <memory>

#include "impl/peer_connection_impl.hpp"

namespace librtc {

class PeerConnectionObserverProxy : public webrtc::PeerConnectionObserver {
 public:
  explicit PeerConnectionObserverProxy(std::weak_ptr<PeerConnectionImpl> impl)
      : impl_(std::move(impl)) {}

  void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {
    if (auto locked = impl_.lock()) {
      locked->handle_signaling_change(convert_signaling_state(new_state));
    }
  }

  void OnIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState new_state) override {
    if (auto locked = impl_.lock()) {
      locked->handle_ice_connection_change(convert_ice_connection_state(new_state));
    }
  }

  void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {
    if (auto locked = impl_.lock()) {
      locked->handle_ice_gathering_change(convert_ice_gathering_state(new_state));
    }
  }

  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override {
    if (auto locked = impl_.lock()) {
      std::string candidate_str;
      candidate->ToString(&candidate_str);
      locked->handle_ice_candidate({.candidate = candidate_str,
                                    .sdp_mid = candidate->sdp_mid(),
                                    .sdp_mline_index = candidate->sdp_mline_index()});
    }
  }

  void OnDataChannel(webrtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {
    if (auto locked = impl_.lock()) {
      locked->handle_data_channel(DataChannelImpl::Create(channel, locked));
    }
  }

  void OnRenegotiationNeeded() override {}
  void OnConnectionChange(webrtc::PeerConnectionInterface::PeerConnectionState) override {}
  void OnStandardizedIceConnectionChange(
      webrtc::PeerConnectionInterface::IceConnectionState) override {}

 private:
  static SignalingState convert_signaling_state(
      webrtc::PeerConnectionInterface::SignalingState state) {
    switch (state) {
      case webrtc::PeerConnectionInterface::kStable:
        return SignalingState::Stable;
      case webrtc::PeerConnectionInterface::kHaveLocalOffer:
        return SignalingState::HaveLocalOffer;
      case webrtc::PeerConnectionInterface::kHaveLocalPrAnswer:
        return SignalingState::HaveLocalPrAnswer;
      case webrtc::PeerConnectionInterface::kHaveRemoteOffer:
        return SignalingState::HaveRemoteOffer;
      case webrtc::PeerConnectionInterface::kHaveRemotePrAnswer:
        return SignalingState::HaveRemotePrAnswer;
      case webrtc::PeerConnectionInterface::kClosed:
        return SignalingState::Closed;
    }
    return SignalingState::Stable;
  }

  static IceConnectionState convert_ice_connection_state(
      webrtc::PeerConnectionInterface::IceConnectionState state) {
    switch (state) {
      case webrtc::PeerConnectionInterface::kIceConnectionNew:
        return IceConnectionState::New;
      case webrtc::PeerConnectionInterface::kIceConnectionChecking:
        return IceConnectionState::Checking;
      case webrtc::PeerConnectionInterface::kIceConnectionConnected:
        return IceConnectionState::Connected;
      case webrtc::PeerConnectionInterface::kIceConnectionCompleted:
        return IceConnectionState::Completed;
      case webrtc::PeerConnectionInterface::kIceConnectionFailed:
        return IceConnectionState::Failed;
      case webrtc::PeerConnectionInterface::kIceConnectionDisconnected:
        return IceConnectionState::Disconnected;
      case webrtc::PeerConnectionInterface::kIceConnectionClosed:
        return IceConnectionState::Closed;
      default:
        return IceConnectionState::New;
    }
  }

  static IceGatheringState convert_ice_gathering_state(
      webrtc::PeerConnectionInterface::IceGatheringState state) {
    switch (state) {
      case webrtc::PeerConnectionInterface::kIceGatheringNew:
        return IceGatheringState::New;
      case webrtc::PeerConnectionInterface::kIceGatheringGathering:
        return IceGatheringState::Gathering;
      case webrtc::PeerConnectionInterface::kIceGatheringComplete:
        return IceGatheringState::Complete;
    }
    return IceGatheringState::New;
  }

  std::weak_ptr<PeerConnectionImpl> impl_;
};

}  // namespace librtc
