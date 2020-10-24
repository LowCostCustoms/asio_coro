#ifndef ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HPP
#define ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HPP

#if defined(__cpp_coroutines)
#include <experimental/coroutine>

namespace asio_coro::detail {
using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;
} // namespace asio_coro::detail
#else // __cpp_coroutines
#if defined(__cpp_impl_coroutine)
#include <coroutine>

namespace asio_coro::detail {
using std::coroutine_handle;
using std::suspend_always;
using std::suspend_never;
} // namespace asio_coro::detail
#else // __cpp_impl_coroutine
#error Compiler does not support coroutines
#endif // __cpp_impl_coroutine
#endif // __cpp_coroutines

#endif // ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HPP
