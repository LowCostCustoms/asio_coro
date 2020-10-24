#ifndef ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP

#include "detail/coroutine_holder.hpp"

#include <boost/asio.hpp>
#include <boost/scope_exit.hpp>
#include <boost/system/error_code.hpp>

#include <utility>

namespace asio_coro {
using async_read_result = std::pair<boost::system::error_code, std::size_t>;

/// Returns an awaitable that suspends the awaiting coroutine, performs one asynchronous read from the specified socket
/// into the specified buffer, and resumes the awaiting coroutine.
///
/// The awaitable returns the pair of values async_read_result, where the first item contains the operation result, and
/// the second - the amount of bytes has been read.
///
/// \param stream   A socket to read data from.
/// \param buffer   A buffer to read data into.
template <class Stream, class MutableBuffer> auto async_read(Stream &stream, const MutableBuffer &buffer) {
  class awaitable {
  public:
    explicit awaitable(Stream &stream, const MutableBuffer &buffer) : _stream(stream), _buffer(buffer) {}

    constexpr bool await_ready() const noexcept { return false; }

    async_read_result await_resume() const noexcept { return _result; }

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      _stream.async_read_some(_buffer, [this, holder = std::move(holder)](const boost::system::error_code &error,
                                                                          std::size_t size) mutable {
        _result = std::make_pair(error, size);
        holder.release().resume();
      });
    }

  private:
    Stream &_stream;
    MutableBuffer _buffer;
    async_read_result _result;
  };

  return awaitable(stream, buffer);
}

/// Returns an awaitable that suspends the awaiting coroutine, performs one or more reads from the specified socket to
/// the specified destination buffer upon either the completion condition is met or an error is occurred during reading
/// the data.
///
/// The awaitable returns a value of type async_read_result, see async_read for details on return value.
///
/// \param stream                   Socket to read data from.
/// \param buffer                   The destination data buffer.
/// \param completion_condition     The read completion condition. See documentation on boost::asio::async_read for
///                                 details.
template <class Stream, class MutableBuffer, class CompletionCondition>
auto async_read(Stream &stream, const MutableBuffer &buffer, const CompletionCondition &completion_condition) {
  class awaitable {
  public:
    explicit awaitable(Stream &stream, const MutableBuffer &buffer, const CompletionCondition &completion_condition)
        : _stream(stream), _buffer(buffer), _completion_condition(completion_condition) {}

    constexpr bool await_ready() const noexcept { return false; }

    async_read_result await_resume() const noexcept { return _result; }

    void await_suspend(detail::coroutine_handle<> continuation) {
      detail::coroutine_holder<> holder(continuation);
      BOOST_SCOPE_EXIT_ALL(&) { holder.release(); };

      boost::asio::async_read(
          _stream, _buffer, _completion_condition,
          [holder = std::move(holder), this](const boost::system::error_code &error, std::size_t size) mutable {
            _result = std::make_pair(error, size);
            holder.release().resume();
          });
    }

  private:
    Stream &_stream;
    MutableBuffer _buffer;
    CompletionCondition _completion_condition;
    async_read_result _result;
  };

  return awaitable(stream, buffer, completion_condition);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_READ_HPP
