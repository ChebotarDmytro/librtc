#pragma once

#include <api/data_channel_interface.h>

#include <librtc/data_channel.hpp>
#include <librtc/utils/event.hpp>
#include <mutex>

namespace librtc {

class DataChannelObserverProxy;

class DataChannelImpl : public DataChannel, public std::enable_shared_from_this<DataChannelImpl> {
 public:
  static std::shared_ptr<DataChannelImpl> Create(
      webrtc::scoped_refptr<webrtc::DataChannelInterface> native,
      std::shared_ptr<void> context = nullptr);

  explicit DataChannelImpl(webrtc::scoped_refptr<webrtc::DataChannelInterface> native,
                           std::shared_ptr<void> context);

  ~DataChannelImpl() override;

  void init();

  // DataChannel Interface Implementation
  Event<MessageBuffer, bool>& on_message() override {
    return message_event;
  }
  Event<DataChannelState>& on_state_change() override {
    return state_event;
  }

  Expected<void> send(MessageBuffer data, bool is_binary) override;
  Expected<void> send(std::string_view text) override;
  Expected<void> send(const std::vector<std::byte>& data) override;

  void close() override;

  std::string label() const override;
  int id() const override;
  uint64_t buffered_amount() const override;
  DataChannelState state() const override;

  // Handlers for proxy
  void handle_state_change();
  void handle_message(const webrtc::DataBuffer& buffer);
  void handle_buffered_amount_change(uint64_t previous_amount) {}

  // Internal event sources
  EventSource<MessageBuffer, bool> message_event;
  EventSource<DataChannelState> state_event;

 private:
  void init_internal();
  static DataChannelState convert_state(webrtc::DataChannelInterface::DataState native_state);

  webrtc::scoped_refptr<webrtc::DataChannelInterface> native_;
  // context_ keeps the parent PeerConnection alive to ensure underlying threads
  // and factory remain valid as long as this DataChannel exists.
  std::shared_ptr<void> context_;
  std::unique_ptr<DataChannelObserverProxy> observer_proxy_;
  mutable std::mutex mutex_;
  DataChannelState cached_state_ = DataChannelState::Closed;
};

}  // namespace librtc
