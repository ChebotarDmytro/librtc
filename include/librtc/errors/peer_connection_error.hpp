#pragma once

#include <string>
#include <system_error>
#include <type_traits>

namespace librtc {

enum class PeerConnectionError {
  Success = 0,
  InvalidArgument,
  InvalidSdp,
  InvalidState,
  InternalError,
  OperationCanceled,
  UnsupportedOperation,
  UnsupportedParameter,
  InvalidRange,
  SyntaxError,
  InvalidModification,
  NetworkError,
  ResourceExhausted,
  OperationError
};

struct PeerConnectionErrorCategory : std::error_category {
  const char* name() const noexcept override {
    return "librtc_peer_connection";
  }
  std::string message(int ev) const override {
    switch (static_cast<PeerConnectionError>(ev)) {
      case PeerConnectionError::Success:
        return "Success";
      case PeerConnectionError::InvalidArgument:
        return "Invalid argument";
      case PeerConnectionError::InvalidSdp:
        return "Invalid SDP";
      case PeerConnectionError::InvalidState:
        return "Invalid state";
      case PeerConnectionError::InternalError:
        return "Internal error";
      case PeerConnectionError::OperationCanceled:
        return "Operation canceled";
      case PeerConnectionError::UnsupportedOperation:
        return "Unsupported operation";
      case PeerConnectionError::UnsupportedParameter:
        return "Unsupported parameter";
      case PeerConnectionError::InvalidRange:
        return "Invalid range";
      case PeerConnectionError::SyntaxError:
        return "Syntax error";
      case PeerConnectionError::InvalidModification:
        return "Invalid modification";
      case PeerConnectionError::NetworkError:
        return "Network error";
      case PeerConnectionError::ResourceExhausted:
        return "Resource exhausted";
      case PeerConnectionError::OperationError:
        return "Operation error";
    }
    return "Unknown error";
  }
};

inline const PeerConnectionErrorCategory& peer_connection_category() {
  static PeerConnectionErrorCategory category;
  return category;
}

inline std::error_code make_error_code(PeerConnectionError e) {
  return {static_cast<int>(e), peer_connection_category()};
}

}  // namespace librtc

namespace std {

template <>
struct is_error_code_enum<librtc::PeerConnectionError> : std::true_type {};

}  // namespace std
