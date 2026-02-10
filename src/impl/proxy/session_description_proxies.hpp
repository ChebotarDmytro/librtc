#pragma once

#include <api/jsep.h>
#include <api/peer_connection_interface.h>
#include <api/rtc_error.h>
#include <api/scoped_refptr.h>
#include <rtc_base/ref_counted_object.h>

#include <functional>
#include <librtc/peer_connection.hpp>
#include <librtc/utils/expected.hpp>
#include <memory>
#include <string>

namespace librtc {
namespace {

PeerConnectionError convert_rtc_error(webrtc::RTCError error) {
  switch (error.type()) {
    case webrtc::RTCErrorType::NONE:
      return PeerConnectionError::Success;
    case webrtc::RTCErrorType::UNSUPPORTED_OPERATION:
      return PeerConnectionError::UnsupportedOperation;
    case webrtc::RTCErrorType::UNSUPPORTED_PARAMETER:
      return PeerConnectionError::UnsupportedParameter;
    case webrtc::RTCErrorType::INVALID_PARAMETER:
      return PeerConnectionError::InvalidArgument;
    case webrtc::RTCErrorType::INVALID_RANGE:
      return PeerConnectionError::InvalidRange;
    case webrtc::RTCErrorType::SYNTAX_ERROR:
      return PeerConnectionError::SyntaxError;
    case webrtc::RTCErrorType::INVALID_STATE:
      return PeerConnectionError::InvalidState;
    case webrtc::RTCErrorType::INVALID_MODIFICATION:
      return PeerConnectionError::InvalidModification;
    case webrtc::RTCErrorType::NETWORK_ERROR:
      return PeerConnectionError::NetworkError;
    case webrtc::RTCErrorType::RESOURCE_EXHAUSTED:
      return PeerConnectionError::ResourceExhausted;
    case webrtc::RTCErrorType::INTERNAL_ERROR:
      return PeerConnectionError::InternalError;
    case webrtc::RTCErrorType::OPERATION_ERROR_WITH_DATA:
      return PeerConnectionError::OperationError;
    default:
      return PeerConnectionError::InternalError;
  }
}

}  // namespace

class CreateDescriptionProxy : public webrtc::CreateSessionDescriptionObserver {
 public:
  using Callback = std::function<void(Result<SessionDescription, PeerConnectionError>)>;

  static webrtc::scoped_refptr<CreateDescriptionProxy> Create(Callback cb) {
    return webrtc::scoped_refptr<CreateDescriptionProxy>(
        new webrtc::RefCountedObject<CreateDescriptionProxy>(std::move(cb)));
  }

  explicit CreateDescriptionProxy(Callback cb) : cb_(std::move(cb)) {}

  void OnSuccess(webrtc::SessionDescriptionInterface* desc) override {
    std::string sdp;
    desc->ToString(&sdp);
    cb_(SessionDescription{.type = webrtc::SdpTypeToString(desc->GetType()), .sdp = sdp});
  }

  void OnFailure(webrtc::RTCError error) override {
    cb_(Err(convert_rtc_error(error)));
  }

 protected:
  ~CreateDescriptionProxy() override = default;

 private:
  Callback cb_;
};

class SetLocalDescriptionProxy : public webrtc::SetLocalDescriptionObserverInterface {
 public:
  using Callback = std::function<void(Result<void, PeerConnectionError>)>;

  static webrtc::scoped_refptr<SetLocalDescriptionProxy> Create(Callback cb) {
    return webrtc::scoped_refptr<SetLocalDescriptionProxy>(
        new webrtc::RefCountedObject<SetLocalDescriptionProxy>(std::move(cb)));
  }

  explicit SetLocalDescriptionProxy(Callback cb) : cb_(std::move(cb)) {}

  void OnSetLocalDescriptionComplete(webrtc::RTCError error) override {
    if (error.ok()) {
      cb_(Success());
    } else {
      cb_(Err(convert_rtc_error(error)));
    }
  }

 protected:
  ~SetLocalDescriptionProxy() override = default;

 private:
  Callback cb_;
};

class SetRemoteDescriptionProxy : public webrtc::SetRemoteDescriptionObserverInterface {
 public:
  using Callback = std::function<void(Result<void, PeerConnectionError>)>;

  static webrtc::scoped_refptr<SetRemoteDescriptionProxy> Create(Callback cb) {
    return webrtc::scoped_refptr<SetRemoteDescriptionProxy>(
        new webrtc::RefCountedObject<SetRemoteDescriptionProxy>(std::move(cb)));
  }

  explicit SetRemoteDescriptionProxy(Callback cb) : cb_(std::move(cb)) {}

  void OnSetRemoteDescriptionComplete(webrtc::RTCError error) override {
    if (error.ok()) {
      cb_(Success());
    } else {
      cb_(Err(convert_rtc_error(error)));
    }
  }

 protected:
  ~SetRemoteDescriptionProxy() override = default;

 private:
  Callback cb_;
};

}  // namespace librtc
