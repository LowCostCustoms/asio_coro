#ifndef ASIO_CORO_EXTENSIONS_TASK_HPP
#define ASIO_CORO_EXTENSIONS_TASK_HPP

#include "detail/task.hpp"

#include <boost/asio.hpp>

namespace asio_coro {
template <class T = void> using task = detail::task<T>;
using void_task = task<void>;

/// Runs coroutine returning task<...> within the specified executor context. Coroutine may be either a function or a
/// functional object. If coroutine is a functional object, the functional object is copied prior to running.
///
/// \param executor   An executor for running the coroutine.
/// \param handler    A coroutine function.
template <class Executor, class Handler> void spawn_coroutine(Executor &executor, Handler &&handler) {
  boost::asio::post(executor, [handler = std::move(handler)]() mutable {
    auto wrapper_coroutine = [](auto &&handler_argument) mutable -> task<void> {
      auto handler = std::move(handler_argument);
      co_await handler();
    };

    auto wrapper_coroutine_task = wrapper_coroutine(std::move(handler));
    wrapper_coroutine_task.release().resume();
  });
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_TASK_HPP
