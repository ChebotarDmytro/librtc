#include "peer_connection_impl.hpp"

#include <api/audio_codecs/builtin_audio_decoder_factory.h>
#include <api/audio_codecs/builtin_audio_encoder_factory.h>
#include <api/create_peerconnection_factory.h>
#include <api/jsep.h>
#include <api/video_codecs/video_decoder_factory_template.h>
#include <api/video_codecs/video_decoder_factory_template_dav1d_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_decoder_factory_template_open_h264_adapter.h>
#include <api/video_codecs/video_encoder_factory_template.h>
#include <api/video_codecs/video_encoder_factory_template_libaom_av1_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp8_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_libvpx_vp9_adapter.h>
#include <api/video_codecs/video_encoder_factory_template_open_h264_adapter.h>
#include <rtc_base/ssl_adapter.h>

#include <librtc/errors/peer_connection_error.hpp>
#include <librtc/utils/async_bridge.hpp>

#include "data_channel_impl.hpp"
#include "proxy/peer_connection_observer_proxy.hpp"
#include "proxy/session_description_proxies.hpp"

namespace librtc {

PeerConnectionImpl::PeerConnectionImpl(std::optional<boost::asio::any_io_executor> executor)
    : executor_(std::move(executor)) {}

PeerConnectionImpl::~PeerConnectionImpl() {
  close();
}

void PeerConnectionImpl::set_threads_and_factory(
    std::unique_ptr<webrtc::Thread> network_thread, std::unique_ptr<webrtc::Thread> worker_thread,
    std::unique_ptr<webrtc::Thread> signaling_thread,
    webrtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> factory) {
  network_thread_ = std::move(network_thread);
  worker_thread_ = std::move(worker_thread);
  signaling_thread_ = std::move(signaling_thread);
  pc_factory_ = std::move(factory);
}

void PeerConnectionImpl::set_pc(webrtc::scoped_refptr<webrtc::PeerConnectionInterface> pc) {
  pc_ = std::move(pc);
}

PeerConnectionImpl::Task<SessionDescription> PeerConnectionImpl::create_offer() {
  co_return co_await AsyncBridge<SessionDescription, PeerConnectionError>::async_run(
      executor_,
      [this](auto cb) {
        pc_->CreateOffer(CreateDescriptionProxy::Create(std::move(cb)).get(),
                         webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
      },
      boost::asio::use_awaitable);
}

PeerConnectionImpl::Task<SessionDescription> PeerConnectionImpl::create_answer() {
  co_return co_await AsyncBridge<SessionDescription, PeerConnectionError>::async_run(
      executor_,
      [this](auto cb) {
        pc_->CreateAnswer(CreateDescriptionProxy::Create(std::move(cb)).get(),
                          webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
      },
      boost::asio::use_awaitable);
}

PeerConnectionImpl::Task<void> PeerConnectionImpl::set_local_description(
    const SessionDescription& sdp) {
  auto sdp_type_res = webrtc::SdpTypeFromString(sdp.type);
  if (!sdp_type_res) {
    co_return Err(PeerConnectionError::InvalidSdp);
  }

  webrtc::SdpParseError error;
  auto session_desc = webrtc::CreateSessionDescription(*sdp_type_res, sdp.sdp, &error);
  if (!session_desc) {
    co_return Err(PeerConnectionError::InvalidSdp);
  }

  co_return co_await AsyncBridge<void, PeerConnectionError>::async_run(
      executor_,
      [this, sd = std::move(session_desc)](auto cb) mutable {
        pc_->SetLocalDescription(std::move(sd), SetLocalDescriptionProxy::Create(std::move(cb)));
      },
      boost::asio::use_awaitable);
}

PeerConnectionImpl::Task<void> PeerConnectionImpl::set_remote_description(
    const SessionDescription& sdp) {
  auto sdp_type_res = webrtc::SdpTypeFromString(sdp.type);
  if (!sdp_type_res) {
    co_return Err(PeerConnectionError::InvalidSdp);
  }

  webrtc::SdpParseError error;
  auto session_desc = webrtc::CreateSessionDescription(*sdp_type_res, sdp.sdp, &error);
  if (!session_desc) {
    co_return Err(PeerConnectionError::InvalidSdp);
  }

  co_return co_await AsyncBridge<void, PeerConnectionError>::async_run(
      executor_,
      [this, sd = std::move(session_desc)](auto cb) mutable {
        pc_->SetRemoteDescription(std::move(sd), SetRemoteDescriptionProxy::Create(std::move(cb)));
      },
      boost::asio::use_awaitable);
}

std::optional<SessionDescription> PeerConnectionImpl::local_description() const {
  if (!pc_ || !pc_->local_description()) return std::nullopt;
  std::string sdp;
  pc_->local_description()->ToString(&sdp);
  return SessionDescription{.type = webrtc::SdpTypeToString(pc_->local_description()->GetType()),
                            .sdp = sdp};
}

std::optional<SessionDescription> PeerConnectionImpl::remote_description() const {
  if (!pc_ || !pc_->remote_description()) return std::nullopt;
  std::string sdp;
  pc_->remote_description()->ToString(&sdp);
  return SessionDescription{.type = webrtc::SdpTypeToString(pc_->remote_description()->GetType()),
                            .sdp = sdp};
}

Expected<void> PeerConnectionImpl::add_ice_candidate(const IceCandidate& candidate) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::IceCandidateInterface> native_candidate(webrtc::CreateIceCandidate(
      candidate.sdp_mid, candidate.sdp_mline_index, candidate.candidate, &error));
  if (!native_candidate) {
    return Err(PeerConnectionError::InvalidArgument);
  }
  if (!pc_->AddIceCandidate(native_candidate.get())) {
    return Err(PeerConnectionError::InternalError);
  }
  return Success();
}

Expected<std::shared_ptr<DataChannel>> PeerConnectionImpl::create_data_channel(
    const std::string& label, const DataChannelConfig& config) {
  webrtc::DataChannelInit init;
  init.ordered = config.ordered;
  if (config.max_retransmits) {
    init.maxRetransmits = *config.max_retransmits;
  }
  if (config.max_retransmit_time_ms) {
    init.maxRetransmitTime = *config.max_retransmit_time_ms;
  }
  init.protocol = config.protocol;
  init.negotiated = config.negotiated;
  if (config.id) {
    init.id = *config.id;
  }

  auto result = pc_->CreateDataChannelOrError(label, &init);
  if (!result.ok()) {
    return Err(PeerConnectionError::InternalError);
  }

  return std::shared_ptr<DataChannel>(
      DataChannelImpl::Create(result.MoveValue(), shared_from_this()));
}

SignalingState PeerConnectionImpl::signaling_state() const {
  std::lock_guard lock(mutex_);
  return cached_signaling_state_;
}

IceConnectionState PeerConnectionImpl::ice_connection_state() const {
  std::lock_guard lock(mutex_);
  return cached_ice_connection_state_;
}

IceGatheringState PeerConnectionImpl::ice_gathering_state() const {
  std::lock_guard lock(mutex_);
  return cached_ice_gathering_state_;
}

void PeerConnectionImpl::close() {
  if (pc_) {
    pc_->Close();
    pc_ = nullptr;
  }
}

void PeerConnectionImpl::handle_signaling_change(SignalingState new_state) {
  {
    std::lock_guard lock(mutex_);
    cached_signaling_state_ = new_state;
  }
  signaling_state_event.emit(new_state);
}

void PeerConnectionImpl::handle_ice_connection_change(IceConnectionState new_state) {
  {
    std::lock_guard lock(mutex_);
    cached_ice_connection_state_ = new_state;
  }
  ice_connection_state_event.emit(new_state);
}

void PeerConnectionImpl::handle_ice_gathering_change(IceGatheringState new_state) {
  {
    std::lock_guard lock(mutex_);
    cached_ice_gathering_state_ = new_state;
  }
}

void PeerConnectionImpl::handle_ice_candidate(const IceCandidate& ice) {
  ice_candidate_event.emit(ice);
}

void PeerConnectionImpl::handle_data_channel(std::shared_ptr<DataChannel> channel) {
  data_channel_event.emit(channel);
}

Expected<std::shared_ptr<PeerConnectionImpl>> PeerConnectionImpl::Create(
    std::optional<boost::asio::any_io_executor> executor, const PeerConnectionConfig& config) {
  // Ensure SSL is initialized exactly once
  static std::once_flag ssl_init_flag;
  std::call_once(ssl_init_flag, []() { webrtc::InitializeSSL(); });

  // Create all 3 threads (matching WebRTCApplication pattern)
  auto network_thread = webrtc::Thread::CreateWithSocketServer();
  auto worker_thread = webrtc::Thread::Create();
  auto signaling_thread = webrtc::Thread::Create();

  network_thread->Start();
  worker_thread->Start();
  signaling_thread->Start();

  webrtc::PeerConnectionFactoryDependencies deps;
  deps.network_thread = network_thread.get();
  deps.worker_thread = worker_thread.get();
  deps.signaling_thread = signaling_thread.get();
  deps.audio_encoder_factory = webrtc::CreateBuiltinAudioEncoderFactory();
  deps.audio_decoder_factory = webrtc::CreateBuiltinAudioDecoderFactory();
  deps.video_encoder_factory = std::make_unique<webrtc::VideoEncoderFactoryTemplate<
      webrtc::LibvpxVp8EncoderTemplateAdapter, webrtc::LibvpxVp9EncoderTemplateAdapter,
      webrtc::OpenH264EncoderTemplateAdapter, webrtc::LibaomAv1EncoderTemplateAdapter>>();
  deps.video_decoder_factory = std::make_unique<webrtc::VideoDecoderFactoryTemplate<
      webrtc::LibvpxVp8DecoderTemplateAdapter, webrtc::LibvpxVp9DecoderTemplateAdapter,
      webrtc::OpenH264DecoderTemplateAdapter, webrtc::Dav1dDecoderTemplateAdapter>>();

  auto pc_factory = webrtc::CreateModularPeerConnectionFactory(std::move(deps));

  if (!pc_factory) {
    return Err(PeerConnectionError::InternalError);
  }

  webrtc::PeerConnectionInterface::RTCConfiguration rtc_config;
  for (const auto& server : config.ice_servers) {
    webrtc::PeerConnectionInterface::IceServer ice_server;
    ice_server.urls = server.urls;
    ice_server.username = server.username;
    ice_server.password = server.credential;
    rtc_config.servers.push_back(ice_server);
  }

  auto impl = std::shared_ptr<PeerConnectionImpl>(new PeerConnectionImpl(std::move(executor)));
  impl->observer_proxy_ = std::make_unique<PeerConnectionObserverProxy>(impl);

  webrtc::PeerConnectionDependencies pc_deps(impl->observer_proxy_.get());
  auto result = pc_factory->CreatePeerConnectionOrError(rtc_config, std::move(pc_deps));

  if (!result.ok()) {
    return Err(PeerConnectionError::InternalError);
  }

  impl->set_threads_and_factory(std::move(network_thread), std::move(worker_thread),
                                std::move(signaling_thread), pc_factory);
  impl->set_pc(result.MoveValue());
  return impl;
}

}  // namespace librtc
