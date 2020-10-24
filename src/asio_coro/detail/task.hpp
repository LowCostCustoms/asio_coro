#ifndef ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP
#define ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP

#include "coroutine_holder.hpp"

#include <cassert>
#include <memory>
#include <variant>

namespace asio_coro::detail {
template <typename ValueType> struct value_holder {
  ValueType value;

  explicit value_holder(const ValueType &value) : value(value) {}

  explicit value_holder(ValueType &&value) : value(std::move(value)) {}
};

template <class ResultType> class task;

template <class ResultType> class task_promise_base {
public:
  /// Default constructor.
  task_promise_base() noexcept = default;

  /// Destroys the associated continuation if any.
  ~task_promise_base() {
    // If the continuation has not been resumed, destroy it.
    if (_continuation) {
      _continuation.destroy();
    }
  }

  /// Always suspends the coroutine.
  constexpr auto initial_suspend() const noexcept { return suspend_always{}; }

  /// Either resumes the continuation if any or resumes the current coroutine.
  constexpr auto final_suspend() noexcept {
    class awaitable {
    public:
      explicit awaitable(coroutine_handle<> continuation) noexcept : _continuation(continuation) {}

      bool await_ready() const noexcept { return !_continuation; }

      constexpr void await_resume() const noexcept {}

      auto await_suspend(coroutine_handle<> continuation) {
        assert(continuation);
        assert(_continuation);

        return _continuation;
      }

    private:
      coroutine_handle<> _continuation;
    };

    // Release the associated continuation to not delete it in destructor by mistake.
    auto continuation = _continuation;
    _continuation = nullptr;

    return awaitable(continuation);
  }

  /// Stores an unhandled exception as the coroutine result.
  void unhandled_exception() {
    assert(std::holds_alternative<none_type>(_value));

    _value.template emplace<exception_type>(std::current_exception());
  }

  /// Sets the continuation.
  void set_continuation(coroutine_handle<> continuation) noexcept {
    assert(!_continuation);
    assert(std::holds_alternative<none_type>(_value));

    _continuation = continuation;
  }

  /// Either returns the coroutine return value or throws the captured exception.
  decltype(auto) get_return_value() {
    assert(!std::holds_alternative<none_type>(_value));

    if (const auto *value = std::get_if<value_type>(&_value); value) {
      return std::move(value->value);
    } else {
      std::rethrow_exception(std::get<exception_type>(_value));
    }
  }

protected:
  /// Stores the specified return value as the coroutine result.
  template <class ValueType> void set_return_value(ValueType &&value) {
    assert(std::holds_alternative<none_type>(_value));

    _value.template emplace<value_type>(std::forward<ValueType>(value));
  }

private:
  using value_type = value_holder<ResultType>;
  using exception_type = std::exception_ptr;
  using none_type = std::monostate;

  coroutine_handle<> _continuation;
  std::variant<std::monostate, exception_type, value_type> _value;
};

template <class ResultType> class task_promise : public task_promise_base<ResultType> {
public:
  using base = task_promise_base<ResultType>;
  using self_type = task_promise<ResultType>;

  template <class ValueType> void return_value(ValueType &&value) {
    base::set_return_value(std::forward<ValueType>(value));
  }

  task<ResultType> get_return_object();
};

template <> class task_promise<void> : public task_promise_base<int> {
public:
  using base = task_promise_base<int>;
  using self_type = task_promise<void>;

  void return_void() { base::set_return_value(0); }

  void get_return_value() { base::get_return_value(); }

  task<void> get_return_object();
};

/// Coroutine handle.
template <class ResultType> class task {
public:
  using promise_type = task_promise<ResultType>;
  using coroutine_handle_type = coroutine_handle<promise_type>;
  using self_type = task<ResultType>;

  /// Default constructor. Creates a task not associated with any coroutine.
  task() noexcept = default;

  /// Constructor. Creates a task from the coroutine handle.
  explicit task(coroutine_handle_type coroutine) noexcept : _coroutine_holder(coroutine) {}

  /// Move constructor. Passes ownership over the associated coroutine from other task handle to the newly created.
  task(self_type &&other) noexcept = default;

  /// Destructor. Destroys the associated coroutine if needed.
  ~task() = default;

  /// Move assignment. Destroys the associated coroutine if any and then passes ownership from other's coroutine to this
  /// task.
  self_type &operator=(self_type &&other) noexcept = default;

  /// Returns an awaitable that resumes the associated task. User must validate the task object prior to calling this
  /// method!
  auto operator co_await() {
    assert(_coroutine_holder.valid());

    class awaitable {
    public:
      explicit awaitable(coroutine_handle_type coroutine) noexcept : _coroutine(coroutine) {
        assert(_coroutine);
        assert(!_coroutine.done());
      }

      ~awaitable() {
        assert(_coroutine);

        // If the associated coroutine has been destroyed via calling the destroy() method on the associated
        // coroutine promise we shouldn't destroy it one more time!
        if (_coroutine.done()) {
          _coroutine.destroy();
        }
      }

      awaitable(awaitable &&other) noexcept : _coroutine(other._coroutine) { other._coroutine = nullptr; }

      constexpr bool await_ready() const noexcept {
        assert(_coroutine);

        return false;
      }

      decltype(auto) await_resume() { return _coroutine.promise().get_return_value(); }

      auto await_suspend(coroutine_handle<> continuation) {
        assert(continuation);
        assert(_coroutine);

        _coroutine.promise().set_continuation(continuation);
        return _coroutine;
      }

    private:
      coroutine_handle_type _coroutine;
    };

    return awaitable(_coroutine_holder.release());
  }

  /// Releases the ownership over the associated coroutine object.
  coroutine_handle_type release() noexcept { return _coroutine_holder.release(); }

  /// Checks whether this task has associated coroutine.
  bool valid() const noexcept { return _coroutine_holder.valid(); }

  /// Destroys the associated coroutine if any.
  void clear() noexcept { _coroutine_holder.clear(); }

private:
  using coroutine_holder_type = coroutine_holder<coroutine_handle<task_promise<ResultType>>>;

  coroutine_holder_type _coroutine_holder;
};

template <class ResultType> task<ResultType> task_promise<ResultType>::get_return_object() {
  return task<ResultType>(coroutine_handle<self_type>::from_promise(*this));
}

inline task<void> task_promise<void>::get_return_object() {
  return task<void>(coroutine_handle<self_type>::from_promise(*this));
}
} // namespace asio_coro::detail

#endif // ASIO_CORO_DETAIL_EXTENSIONS_TASK_HPP
