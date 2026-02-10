#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <chrono>
#include <iostream>
#include <librtc/data_channel.hpp>
#include <librtc/peer_connection.hpp>
#include <memory>
#include <thread>
#include <vector>

using namespace librtc;
using namespace std::chrono_literals;
namespace asio = boost::asio;

struct Peer : std::enable_shared_from_this<Peer> {
  std::string name;
  std::shared_ptr<PeerConnection> pc;
  std::shared_ptr<DataChannel> dc;
  std::vector<IceCandidate> pending_ice;
  std::atomic<bool> connected{false};
  std::atomic<bool> message_received{false};

  Peer(std::string n) : name(std::move(n)) {}

  asio::awaitable<void> initialize() {
    auto pc_res = PeerConnection::Create();
    if (!pc_res) {
      co_return;
    }
    pc = pc_res.value();

    auto self = weak_from_this();

    pc->on_ice_candidate(self, [](auto& p, const IceCandidate& c) { p.pending_ice.push_back(c); });

    pc->on_ice_connection_state_change(self, [](auto& p, IceConnectionState state) {
      std::cout << "[" << p.name << "] ICE: " << (int)state << "\n";
      if (state == IceConnectionState::Connected) {
        p.connected = true;
      }
    });

    pc->on_data_channel(self, [](auto& p, auto channel) { p.setup_dc(channel); });

    co_return;
  }

  void setup_dc(std::shared_ptr<DataChannel> channel) {
    dc = channel;
    auto self = weak_from_this();

    dc->on_state_change(self, [](auto& p, DataChannelState state) {
      if (state == DataChannelState::Open) {
        (void)p.dc->send("Hello from " + p.name);
      }
    });

    dc->on_message(self, [](auto& p, auto buffer, bool) {
      std::cout << "[" << p.name
                << "] Recv: " << std::string_view((char*)buffer.data(), buffer.size()) << "\n";
      p.message_received = true;
    });
  }

  void stop() {
    if (dc) {
      dc->close();
      dc.reset();
    }
    if (pc) {
      pc->close();
      pc.reset();
    }
  }
};

asio::awaitable<void> run_test() {
  auto executor = co_await asio::this_coro::executor;

  auto alice = std::make_shared<Peer>("Alice");
  auto bob = std::make_shared<Peer>("Bob");

  co_await alice->initialize();
  co_await bob->initialize();

  // Alice creates data channel
  auto dc_res = alice->pc->create_data_channel("test");
  if (dc_res) {
    alice->setup_dc(dc_res.value());
  }

  // SDP Exchange
  auto offer_res = co_await alice->pc->create_offer();
  if (!offer_res) {
    std::cerr << "Offer failed\n";
    alice->stop();
    bob->stop();
    co_return;
  }

  (void)co_await alice->pc->set_local_description(offer_res.value());
  (void)co_await bob->pc->set_remote_description(offer_res.value());

  auto answer_res = co_await bob->pc->create_answer();
  if (!answer_res) {
    std::cerr << "Answer failed\n";
    alice->stop();
    bob->stop();
    co_return;
  }

  (void)co_await bob->pc->set_local_description(answer_res.value());
  (void)co_await alice->pc->set_remote_description(answer_res.value());

  // ICE Exchange
  asio::steady_timer timer(executor);
  for (int i = 0; i < 5; ++i) {
    timer.expires_after(100ms);
    co_await timer.async_wait(asio::use_awaitable);
    for (const auto& c : alice->pending_ice) (void)bob->pc->add_ice_candidate(c);
    for (const auto& c : bob->pending_ice) (void)alice->pc->add_ice_candidate(c);
    alice->pending_ice.clear();
    bob->pending_ice.clear();
  }

  // Wait for results
  int retries = 50;
  while (retries-- > 0 && !(alice->message_received && bob->message_received)) {
    timer.expires_after(100ms);
    co_await timer.async_wait(asio::use_awaitable);
  }

  if (alice->message_received && bob->message_received) {
    std::cout << "SUCCESS! Coroutines are working.\n";
  } else {
    std::cerr << "Test timed out\n";
  }

  // CRITICAL: Explicitly shutdown and NULLIFY pointers while the coroutine is
  // still running. This ensures destruction happens on the main thread while
  // background threads are healthy.
  alice->stop();
  bob->stop();
  alice.reset();
  bob.reset();

  // Final grace period for background cleanup
  timer.expires_after(1000ms);
  co_await timer.async_wait(asio::use_awaitable);

  std::cout << "Exiting test coroutine cleanly.\n";
}

int main() {
  try {
    asio::io_context ctx;
    asio::co_spawn(ctx, run_test(), asio::detached);
    ctx.run();
    std::cout << "Main loop exited.\n";
  } catch (const std::exception& e) {
    std::cerr << "Exception in main: " << e.what() << "\n";
  }
  return 0;
}
