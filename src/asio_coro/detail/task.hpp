#ifndef ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP
#define ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP

#include "coroutine_holder.hpp"

#include <variant>
#include <cassert>
#include <memory>

namespace asio_coro::detail {
/**
 * Represents an unset return value. Values of this type must never be used as the coroutine result.
 */
class none {
public:
    constexpr none() = default;
};

/**
 * Stores an exception thrown during coroutine execution. Values of this type must never be used as the
 * coroutine result.
 */
class exception_wrapper {
public:
    explicit exception_wrapper(const std::exception_ptr &exception)
            : _exception(exception) {
    }

    [[noreturn]] void rethrow_exception() const {
        std::rethrow_exception(_exception);
    }

private:
    std::exception_ptr _exception;
};

template<class ResultType>
class task;

template<class ResultType>
class task_promise_base {
public:
    /**
     * Default constructor.
     */
    task_promise_base() noexcept
            : _continuation(nullptr) {
    }

    /**
     * Destructor.
     */
    ~task_promise_base() {
        if (_continuation) {
            _continuation.destroy();
        }
    }

    /**
     * Suspends the coroutine always.
     */
    constexpr auto initial_suspend() const noexcept {
        return std::suspend_always {};
    }

    /**
     * Doesn't suspend the coroutine.
     */
    constexpr auto final_suspend() const noexcept {
        return std::suspend_never {};
    }

    /**
     * Stores the specified return value as the coroutine result.
     */
    template<class ValueType>
    void return_value(ValueType &&value) {
        assert(std::holds_alternative<none>(_value));

        _value.template emplace<ResultType>(std::forward<ValueType>(value));
        if (const auto continuation = release_continuation(); continuation != nullptr) {
            continuation.resume();
        }
    }

    /**
     * Stores an unhandled exception as the coroutine result.
     */
    void unhandled_exception() {
        assert(std::holds_alternative<none>(_value));

        _value.template emplace<exception_wrapper>(std::current_exception());
        if (const auto continuation = release_continuation(); continuation != nullptr) {
            continuation.resume();
        }
    }

    /**
     * Attempts to set coroutine continuation.
     *
     * @return True if continuation has been set or false otherwise.
     */
    bool set_continuation(std::coroutine_handle<> continuation) {
        assert(!_continuation);

        if (std::holds_alternative<none>(_value)) {
            _continuation = continuation;
            return true;
        }

        return false;
    }

    /**
     * Either returns the coroutine return value or throws the captured exception.
     */
    decltype(auto) get_return_value() {
        assert(!std::holds_alternative<none>(_value));

        if (const auto *value = std::get_if<ResultType>(&_value); value != nullptr) {
            return std::move(*value);
        } else {
            std::get_if<exception_wrapper>(&_value)->rethrow_exception();
        }
    }

private:
    std::coroutine_handle<> _continuation;
    std::variant<none, exception_wrapper, ResultType> _value;

    std::coroutine_handle<> release_continuation() noexcept {
        const auto continuation = _continuation;
        _continuation = nullptr;

        return continuation;
    }
};

template<class ResultType>
class task_promise : public task_promise_base<ResultType> {
public:
    using base = task_promise_base<ResultType>;
    using self_type = task_promise<ResultType>;

    template<class ValueType>
    void return_value(ValueType &&value) {
        base::return_value(std::forward<ValueType>(value));
    }

    task<ResultType> get_return_object();
};

template<>
class task_promise<void> : public task_promise_base<int> {
public:
    using base = task_promise_base<int>;
    using self_type = task_promise<void>;

    void return_void() {
        base::return_value(0);
    }

    void get_return_value() {
        base::get_return_value();
    }

    task<void> get_return_object();
};

/**
 * A simple coroutine handle.
 */
template<class ResultType>
class task : public coroutine_holder<std::coroutine_handle<task_promise<ResultType>>> {
public:
    using base = coroutine_holder<std::coroutine_handle<task_promise<ResultType>>>;
    using promise_type = task_promise<ResultType>;
    using coroutine_handle_type = std::coroutine_handle<promise_type>;
    using self_type = task<ResultType>;


    /**
     * Default constructor. Creates a task not associated with any coroutine.
     */
    task() noexcept = default;

    /**
     * Constructor. Creates a task from the coroutine handle.
     */
    task(coroutine_handle_type coroutine) noexcept
            : base(coroutine) {
    }

    /**
     * Move constructor. Passes ownership over the associated coroutine from other task handle to the newly created.
     */
    task(self_type &&other) noexcept = default;

    /**
     * Destructor. Destroys the associated coroutine if needed.
     */
    ~task() = default;

    /**
     * Move assignment. Destroys the associated coroutine if any and then passes ownership from other's coroutine to
     * this task.
     */
    self_type &operator=(self_type &&other) noexcept = default;

    /**
     * Returns an awaitable that resumes the associated task.
     */
    auto operator co_await() {
        assert(base::_coroutine);

        class awaitable {
        public:
            explicit awaitable(coroutine_handle_type coroutine) noexcept
                    : _coroutine(coroutine) {
            }

            bool await_ready() const noexcept {
                return _coroutine.done();
            }

            decltype(auto) await_resume() {
                return _coroutine.promise().get_return_value();
            }

            bool await_suspend(std::coroutine_handle<> continuation) {
                if (_coroutine.promise().set_continuation(continuation)) {
                    _coroutine.resume();
                    return true;
                } else {
                    return false;
                }
            }

        private:
            coroutine_handle_type _coroutine;
        };

        return awaitable(base::_coroutine);
    }
};

template<class ResultType>
task<ResultType> task_promise<ResultType>::get_return_object() {
    return task<ResultType>(std::coroutine_handle<self_type>::from_promise(*this));
}

inline task<void> task_promise<void>::get_return_object() {
    return task<void>(std::coroutine_handle<self_type>::from_promise(*this));
}
} // namespace asio_coro::detail

#endif // ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP
