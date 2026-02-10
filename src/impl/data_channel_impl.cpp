#include "data_channel_impl.hpp"

#include <rtc_base/copy_on_write_buffer.h>

#include <librtc/errors/data_channel_error.hpp>
#include <memory>
#include <string>

#include "proxy/data_channel_observer_proxy.hpp"

namespace librtc {

std::shared_ptr<DataChannelImpl> DataChannelImpl::Create(
    webrtc::scoped_refptr<webrtc::DataChannelInterface> native, std::shared_ptr<void> context) {
  auto impl = std::make_shared<DataChannelImpl>(std::move(native), std::move(context));
  impl->init();
  return impl;
}

DataChannelImpl::DataChannelImpl(webrtc::scoped_refptr<webrtc::DataChannelInterface> native,
                                 std::shared_ptr<void> context)
    : native_(std::move(native)),
      context_(std::move(context)) {}  // context_ acts as a lifetime anchor for the parent PC

DataChannelImpl::~DataChannelImpl() {
  if (native_) {
    native_->Close();
    native_->UnregisterObserver();
    native_ = nullptr;
  }
}

void DataChannelImpl::init() {
  init_internal();
}

void DataChannelImpl::init_internal() {
  if (native_) {
    observer_proxy_ = std::make_unique<DataChannelObserverProxy>(weak_from_this());
    native_->RegisterObserver(observer_proxy_.get());
    cached_state_ = convert_state(native_->state());
  }
}

void DataChannelImpl::handle_state_change() {
  auto new_state = convert_state(native_->state());

  {
    std::lock_guard lock(mutex_);
    cached_state_ = new_state;
  }

  state_event.emit(new_state);
}

void DataChannelImpl::handle_message(const webrtc::DataBuffer& buffer) {
  DataChannel::MessageBuffer data{reinterpret_cast<const std::byte*>(buffer.data.data()),
                                  buffer.data.size()};

  message_event.emit(data, buffer.binary);
}

Expected<void> DataChannelImpl::send(MessageBuffer data, bool is_binary) {
  if (!native_) {
    return Err(DataChannelError::Closed);
  }

  if (native_->state() != webrtc::DataChannelInterface::kOpen) {
    return Err(DataChannelError::NotOpen);
  }

  webrtc::DataBuffer buffer(
      webrtc::CopyOnWriteBuffer(reinterpret_cast<const uint8_t*>(data.data()), data.size()),
      is_binary);

  if (native_->Send(buffer)) {
    return Success();
  } else {
    return Err(DataChannelError::BufferFull);
  }
}

Expected<void> DataChannelImpl::send(std::string_view text) {
  return send({reinterpret_cast<const std::byte*>(text.data()), text.size()}, false);
}

Expected<void> DataChannelImpl::send(const std::vector<std::byte>& data) {
  return send({data.data(), data.size()}, true);
}

void DataChannelImpl::close() {
  if (native_) {
    native_->Close();
  }
}

std::string DataChannelImpl::label() const {
  if (native_) {
    return native_->label();
  }

  return "";
}

int DataChannelImpl::id() const {
  if (native_) {
    return native_->id();
  }

  return -1;
}

uint64_t DataChannelImpl::buffered_amount() const {
  if (native_) {
    return native_->buffered_amount();
  }

  return 0;
}

DataChannelState DataChannelImpl::state() const {
  std::lock_guard lock(mutex_);
  return cached_state_;
}

DataChannelState DataChannelImpl::convert_state(
    webrtc::DataChannelInterface::DataState native_state) {
  switch (native_state) {
    case webrtc::DataChannelInterface::kConnecting:
      return DataChannelState::Connecting;
    case webrtc::DataChannelInterface::kOpen:
      return DataChannelState::Open;
    case webrtc::DataChannelInterface::kClosing:
      return DataChannelState::Closing;
    case webrtc::DataChannelInterface::kClosed:
      return DataChannelState::Closed;
  }

  return DataChannelState::Closed;
}

}  // namespace librtc
