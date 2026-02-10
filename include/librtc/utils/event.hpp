#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

namespace librtc {

/**
 * Helper macro to declare an event in an interface.
 * Defines both the virtual getter and the template helper for convenience.
 */
#define EVENT(name, ...)                                       \
  virtual Event<__VA_ARGS__>& on_##name() = 0;                 \
  template <typename T, typename F>                            \
  void on_##name(std::weak_ptr<T> context, F&& handler) {      \
    on_##name()(std::move(context), std::forward<F>(handler)); \
  }

/**
 * Event is the public interface for subscribing to events.
 */
template <typename... Args>
class Event {
 public:
  virtual ~Event() = default;

  /**
   * Subscribe to the event with a context.
   * The subscription is automatically removed when the context (tracker) is destroyed.
   * \param context The weak_ptr to track.
   * \param handler The callback function(T& context, Args...).
   */
  template <typename T, typename F>
  void connect(std::weak_ptr<T> context, F&& handler) {
    subscribe_internal(context,
                       [context, handler = std::forward<F>(handler)](Args... args) mutable {
                         if (auto locked = context.lock()) {
                           handler(*locked, std::forward<Args>(args)...);
                         }
                       });
  }

  /**
   * Shortcut for connect.
   */
  template <typename T, typename F>
  void operator()(std::weak_ptr<T> context, F&& handler) {
    connect(std::move(context), std::forward<F>(handler));
  }

 protected:
  virtual void subscribe_internal(std::weak_ptr<void> tracker,
                                  std::function<void(Args...)> handler) = 0;
};

/**
 * EventSource manages a list of subscriptions and can emit events.
 * It implements the Event interface for subscription.
 */
template <typename... Args>
class EventSource : public Event<Args...> {
 public:
  using Handler = std::function<void(Args...)>;

  struct Subscription {
    std::weak_ptr<void> tracker;
    Handler handler;
  };

  void emit(Args... args) {
    std::vector<Handler> targets;
    {
      std::lock_guard lock(mutex_);
      for (auto it = subscriptions_.begin(); it != subscriptions_.end();) {
        if (it->tracker.expired()) {
          it = subscriptions_.erase(it);
        } else {
          targets.push_back(it->handler);
          ++it;
        }
      }
    }

    for (auto& handler : targets) {
      handler(std::forward<Args>(args)...);
    }
  }

 protected:
  void subscribe_internal(std::weak_ptr<void> tracker, Handler handler) override {
    std::lock_guard lock(mutex_);
    subscriptions_.push_back({std::move(tracker), std::move(handler)});
  }

 private:
  std::mutex mutex_;
  std::vector<Subscription> subscriptions_;
};

}  // namespace librtc
