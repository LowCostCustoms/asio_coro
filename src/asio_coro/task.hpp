#ifndef ASIO_CORO_EXTENSIONS_TASK_HPP
#define ASIO_CORO_EXTENSIONS_TASK_HPP

#include "detail/task.hpp"

#include <boost/asio.hpp>

namespace asio_coro {
template<class T>
using task = detail::task<T>;

/**
 * Spawns a coroutine within the specified executor context using boost::asio::post helper.
 *
 * @param executor  Executor context.
 * @param handler   Coroutine handler function/functional object. Must return at awaitable object.
 */
template<class Executor, class Handler>
void spawn_coroutine(Executor &executor, Handler &&handler) {
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
