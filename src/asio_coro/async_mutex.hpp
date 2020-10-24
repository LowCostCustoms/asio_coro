#ifndef ASIO_CORO_EXTENSIONS_ASYNC_MUTEX_HPP
#define ASIO_CORO_EXTENSIONS_ASYNC_MUTEX_HPP

#include "detail/coroutine.hpp"

#include <cassert>
#include <mutex>

namespace asio_coro {
namespace detail {
template <typename MutexType> class async_mutex_lock {
public:
  using self_type = async_mutex_lock<MutexType>;

  /// Default constructor. Creates an instance of async_mutex_lock that doesn't own any lock.
  constexpr async_mutex_lock() noexcept : m_mutex(nullptr) {}

  /// Move constructor. Transfers ownership over the associated lock from other lock to the newly created lock.
  explicit constexpr async_mutex_lock(self_type &&other) noexcept : m_mutex(other.m_mutex) { other.m_mutex = nullptr; }

  /// Constructor. Creates an instance of async_mutex_lock assuming the lock over the mutex has already been acquired.
  explicit constexpr async_mutex_lock(MutexType &mutex, std::adopt_lock_t) noexcept : m_mutex(&mutex) {}

  /// Destructor. Resumes pending coroutine if any.
  ~async_mutex_lock() {
    if (m_mutex) {
      m_mutex->unlock();
    }
  }

  /// Move assignment. Unlocks this lock if needed, transfers ownership over the associated lock from other to this
  /// async_mutex_lock.
  self_type &operator=(self_type &&other) noexcept {
    if (std::addressof(other) != this) {
      if (m_mutex) {
        m_mutex->unlock();
      }

      m_mutex = other.m_mutex;
      other.m_mutex = nullptr;
    }

    return *this;
  }

  /// Returns true if this object owns a lock over any mutex.
  bool owns_lock() const noexcept { return m_mutex != nullptr; }

private:
  MutexType *m_mutex;
};

template <typename MutexType> class async_mutex_lock_operation {
public:
  constexpr async_mutex_lock_operation(MutexType &mutex) noexcept : m_mutex(mutex) {}

  constexpr bool await_ready() const noexcept { return false; }

  constexpr void await_resume() const noexcept {}

  bool await_suspend(detail::coroutine_handle<> continuation) {
    m_item.continuation = continuation;
    return !m_mutex.try_lock(&m_item);
  }

protected:
  MutexType &m_mutex;
  typename MutexType::continuations_list_item m_item;
};

template <typename MutexType> class scoped_async_mutex_lock_operation : public async_mutex_lock_operation<MutexType> {
public:
  constexpr scoped_async_mutex_lock_operation(MutexType &mutex) noexcept
      : async_mutex_lock_operation<MutexType>(mutex) {}

  constexpr auto await_resume() noexcept {
    return async_mutex_lock<MutexType>(async_mutex_lock_operation<MutexType>::m_mutex, std::adopt_lock_t());
  }
};
}; // namespace detail

class async_mutex;
using async_mutex_lock = detail::async_mutex_lock<async_mutex>;

/// An asynchronous mutex.
class async_mutex {
public:
  /// Constructor. Creates an instance of async_mutex.
  async_mutex() : m_locked(false), m_first(nullptr), m_last(nullptr) {}

  /// Destructor.
  ~async_mutex() {
    // There shouldn't be any awaiting coroutines upon the async_mutex destruction!
    assert(!m_locked);
    assert(!m_first);
    assert(!m_last);
  }

  /// Unlocks the mutex. Resumes a pending coroutine if there are any.
  void unlock() {
    continuations_list_item *item;
    {
      std::lock_guard lock(m_mutex);

      assert(m_locked);

      if (!m_first) {
        assert(!m_last);

        m_locked = false;
        return;
      }

      assert(m_last);

      item = m_first;
      if (m_first == m_last) {
        m_first = nullptr;
        m_last = nullptr;
      } else {
        m_first = m_first->next;
      }
    }

    assert(item);
    assert(item->continuation);

    item->continuation.resume();
  }

  /// Attempts to lock the mutex. Returns true on successfull outcome, false otherwise.
  bool try_lock() {
    std::lock_guard lock(m_mutex);
    if (m_locked) {
      return false;
    }

    m_locked = true;
    return true;
  }

  /// Returns an awaitable that suspends the awaiting coroutine until the lock is acquired.
  constexpr auto async_lock() noexcept { return detail::async_mutex_lock_operation<async_mutex>(*this); }

  /// Returns an awaitable that suspends the awaiting coroutine until the lock is acquired.
  ///
  /// The awaitable returns an instance of async_mutex_lock that holds lock over this mutex.
  constexpr auto async_lock_scoped() noexcept { return detail::scoped_async_mutex_lock_operation<async_mutex>(*this); }

private:
  friend detail::async_mutex_lock_operation<async_mutex>;

  /// An element of singly-linked list of awaiting continuations.
  struct continuations_list_item {
    continuations_list_item *next = nullptr;
    detail::coroutine_handle<> continuation;
  };

  std::mutex m_mutex;
  bool m_locked;
  continuations_list_item *m_first;
  continuations_list_item *m_last;

  /// Attempts to lock the mutex, returns true if lock is acquired, returns false and adds item to the list of awaiters
  /// otherwise.
  bool try_lock(continuations_list_item *item) {
    std::lock_guard lock(m_mutex);

    if (!m_locked) {
      m_locked = true;
      return true;
    }

    if (m_last) {
      m_last->next = item;
    } else {
      assert(!m_first);
      m_first = item;
    }

    m_last = item;

    return false;
  }
};
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_ASYNC_MUTEX_HPP
