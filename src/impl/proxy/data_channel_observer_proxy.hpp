#pragma once

#include <api/data_channel_interface.h>

#include <memory>

#include "impl/data_channel_impl.hpp"

namespace librtc {

class DataChannelObserverProxy : public webrtc::DataChannelObserver {
 public:
  explicit DataChannelObserverProxy(std::weak_ptr<DataChannelImpl> impl) : impl_(std::move(impl)) {}

  void OnStateChange() override {
    if (auto locked = impl_.lock()) {
      locked->handle_state_change();
    }
  }

  void OnMessage(const webrtc::DataBuffer& buffer) override {
    if (auto locked = impl_.lock()) {
      locked->handle_message(buffer);
    }
  }

  void OnBufferedAmountChange(uint64_t previous_amount) override {
    if (auto locked = impl_.lock()) {
      locked->handle_buffered_amount_change(previous_amount);
    }
  }

 private:
  std::weak_ptr<DataChannelImpl> impl_;
};

}  // namespace librtc
