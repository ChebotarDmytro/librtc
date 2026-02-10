#pragma once

#include <boost/asio/any_io_executor.hpp>
#include <boost/asio/associated_executor.hpp>
#include <boost/asio/async_result.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <librtc/utils/expected.hpp>
#include <memory>
#include <optional>
#include <system_error>

namespace librtc {

/**
 * A stable, production-ready utility to bridge WebRTC callbacks to Asio's
 * universal asynchronous model (including coroutines).
 */
template <typename T, typename E = std::error_code>
class AsyncBridge {
 public:
  /**
   * Initiates the operation. This is the entry point for co_await.
   */
  template <typename CompletionToken, typename Func>
  static auto async_run(Func&& initiate_func, CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, void(Result<T, E>)>(
        [initiate_func = std::forward<Func>(initiate_func)](auto handler) mutable {
          // Get the executor associated with the handler (e.g., from use_awaitable)
          auto executor = boost::asio::get_associated_executor(handler);

          // Wrap the handler in a shared pointer to allow it to be safely
          // called from the WebRTC signaling thread.
          auto shared_handler = std::make_shared<decltype(handler)>(std::move(handler));

          auto callback = [shared_handler, executor](Result<T, E> res) mutable {
            // Marshall the result back to the application thread/executor.
            boost::asio::post(executor, [h = std::move(*shared_handler),
                                         r = std::move(res)]() mutable { h(std::move(r)); });
          };

          initiate_func(std::move(callback));
        },
        token);
  }

  /**
   * Version that takes an optional executor.
   */
  template <typename CompletionToken, typename Func>
  static auto async_run(std::optional<boost::asio::any_io_executor> executor, Func&& initiate_func,
                        CompletionToken&& token) {
    return boost::asio::async_initiate<CompletionToken, void(Result<T, E>)>(
        [initiate_func = std::forward<Func>(initiate_func), executor](auto handler) mutable {
          // If no executor was explicitly provided, try to get it from the handler.
          auto exec = executor ? *executor : boost::asio::get_associated_executor(handler);

          auto shared_handler = std::make_shared<decltype(handler)>(std::move(handler));
          auto callback = [shared_handler, exec](Result<T, E> res) mutable {
            boost::asio::post(exec, [h = std::move(*shared_handler), r = std::move(res)]() mutable {
              h(std::move(r));
            });
          };
          initiate_func(std::move(callback));
        },
        token);
  }

  /**
   * Legacy version that takes an explicit executor.
   */
  template <typename CompletionToken, typename Func>
  static auto async_run(boost::asio::any_io_executor executor, Func&& initiate_func,
                        CompletionToken&& token) {
    return async_run(std::make_optional(executor), std::forward<Func>(initiate_func),
                     std::forward<CompletionToken>(token));
  }
};

}  // namespace librtc
