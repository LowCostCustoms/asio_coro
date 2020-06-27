#ifndef ASIO_CORO_EXTENSIONS_ASYNC_WRITE_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_WRITE_HPP

#include "detail/coroutine_holder.hpp"
#include "detail/concepts.hpp"

#include <boost/system/error_code.hpp>
#include <boost/scope_exit.hpp>

#include <utility>

namespace asio_coro {
using async_write_result = std::pair<boost::system::error_code, std::size_t>;

/**
 * Returns an awaitable.
 */
template<class Stream, class Buffer>
auto async_write(Stream &stream, const Buffer &buffer) {
    class awaitable {
    public:
        explicit awaitable(Stream &stream, const Buffer &buffer)
                : _stream(stream), _buffer(buffer) {
        }

        bool await_ready() const noexcept {
            return false;
        }

        async_write_result await_resume() const noexcept {
            return _result;
        }

        void await_suspend(std::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            _stream.async_write_some(_buffer,
                    [this, holder = std::move(holder)](const boost::system::error_code &error,
                            std::size_t size) mutable {
                        _result = std::make_pair(error, size);
                        holder.release().resume();
                    });
        }

    private:
        Stream &_stream;
        Buffer _buffer;
        async_write_result _result;
    };

    return awaitable(stream, buffer);
}

template<class Stream, class Buffer, class CompletionCondition>
auto async_write(Stream &stream,
        const Buffer &buffer,
        const CompletionCondition &completion_condition) requires detail::completion_condition<CompletionCondition> {
    class awaitable {
    public:
        explicit awaitable(Stream &stream, const Buffer &buffer, const CompletionCondition &completion_condition)
                : _stream(stream), _buffer(buffer), _completion_condition(completion_condition) {
        }

        constexpr bool await_ready() const noexcept {
            return false;
        }

        async_write_result await_resume() const noexcept {
            return _result;
        }

        void await_suspend(std::coroutine_handle<> continuation) {
            detail::coroutine_holder<> holder(continuation);
            BOOST_SCOPE_EXIT_ALL(&) {
                holder.release();
            };

            boost::asio::async_write(_stream,
                    _buffer,
                    _completion_condition,
                    [this, holder = std::move(holder)](const boost::system::error_code &error,
                            std::size_t size) mutable {
                        _result = std::make_pair(error, size);
                        holder.release().resume();
                    });
        }

    private:
        Stream &_stream;
        Buffer _buffer;
        CompletionCondition _completion_condition;
        async_write_result _result;
    };

    return awaitable(stream, buffer, completion_condition);
}
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_WRITE_HPP
