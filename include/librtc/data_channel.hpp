#pragma once

#include <librtc/utils/event.hpp>
#include <librtc/utils/expected.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace librtc {

enum class DataChannelState { Connecting, Open, Closing, Closed };

struct DataChannelConfig {
  bool ordered = true;
  std::optional<int> max_retransmit_time_ms;
  std::optional<int> max_retransmits;
  std::string protocol;
  bool negotiated = false;
  std::optional<int> id;
};

class DataChannel {
 public:
  using MessageBuffer = std::span<const std::byte>;

  virtual ~DataChannel() = default;

  // Event handlers
  EVENT(message, MessageBuffer, bool)
  EVENT(state_change, DataChannelState)

  // Actions
  virtual Expected<void> send(MessageBuffer data, bool is_binary) = 0;
  virtual Expected<void> send(std::string_view text) = 0;
  virtual Expected<void> send(const std::vector<std::byte>& data) = 0;

  virtual void close() = 0;

  // Properties
  virtual std::string label() const = 0;
  virtual int id() const = 0;
  virtual uint64_t buffered_amount() const = 0;
  virtual DataChannelState state() const = 0;
};

}  // namespace librtc
