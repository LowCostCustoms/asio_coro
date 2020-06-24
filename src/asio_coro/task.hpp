#ifndef ASIO_CORO_EXTENSIONS_TASK_HPP
#define ASIO_CORO_EXTENSIONS_TASK_HPP

#include "detail/task.hpp"

#include <boost/asio.hpp>

#include <concepts>
#include <functional>

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
        // Create a wrapper coroutine that will ensure the handler won't be destroyed until the coroutine has finished.
        auto wrapper = std::bind([](auto &&handler) mutable -> task<void> {
            auto local_handler = std::move(handler);
            co_await local_handler();
        }, std::move(handler));

        auto wrapper_task = wrapper();
        wrapper_task.release().resume();
    });
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_TASK_HPP
