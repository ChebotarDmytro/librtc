#pragma once

#include <string>
#include <system_error>
#include <type_traits>

namespace librtc {

enum class DataChannelError {
  Success = 0,
  NotOpen = 1,
  BufferFull,
  InvalidArgument,
  InvalidData,
  Closed
};

struct DataChannelErrorCategory : std::error_category {
  const char* name() const noexcept override {
    return "librtc_datachannel";
  }
  std::string message(int ev) const override {
    switch (static_cast<DataChannelError>(ev)) {
      case DataChannelError::Success:
        return "Success";
      case DataChannelError::NotOpen:
        return "Data channel is not open";
      case DataChannelError::BufferFull:
        return "Send buffer is full";
      case DataChannelError::Closed:
        return "Data channel is closed";
      case DataChannelError::InvalidArgument:
        return "Invalid argument";
      case DataChannelError::InvalidData:
        return "Invalid data";
    }

    return "Unknown error";
  }
};

inline const DataChannelErrorCategory& data_channel_category() {
  static DataChannelErrorCategory category;
  return category;
}

inline std::error_code make_error_code(DataChannelError e) {
  return {static_cast<int>(e), data_channel_category()};
}

}  // namespace librtc

namespace std {

template <>
struct is_error_code_enum<librtc::DataChannelError> : std::true_type {};

}  // namespace std
