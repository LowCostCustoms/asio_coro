#ifndef ASIO_CORO_EXTENSIONS_MUTEX_HPP
#define ASIO_CORO_EXTENSIONS_MUTEX_HPP

#include "detail/coroutine_holder.hpp"

#include <mutex>
#include <vector>

namespace asio_coro {
template<class Mutex>
class unique_lock {
public:
    using mutex_type = Mutex;
    using self_type = unique_lock<Mutex>;

    /**
     * Default constructor. Creates an unique lock that isn't associated with any mutex.
     */
    unique_lock()
            : _mutex(nullptr), _locked(false) {
    }

    /**
     * Creates an unique lock associated with the specified mutex.
     *
     * @param mutex     Asynchronous mutex.
     * @param locked    True if the lock on the mutex has been acquired and its ownership should be passed to the newly
     *                  created instance of unique lock.
     */
    explicit unique_lock(Mutex &mutex, bool locked = false)
            : _mutex(&mutex), _locked(locked) {
    }

    /**
     * Move constructor. Transfers lock ownership from other unique lock to the newly created instance of unique lock.
     */
    unique_lock(self_type &&other) noexcept
            : _mutex(&other._mutex), _locked(other._locked) {
        other._locked = false;
    }

    /**
     * Destructor. Unlocks the lock if needed.
     */
    ~unique_lock() {
        unlock();
    }

    /**
     * Move assignment. Unlocks the current lock if needed. Transfers lock ownership from other unique lock to this
     * unique lock.
     */
    self_type &operator=(self_type &&other) {
        if (std::addressof(other) != this) {
            unlock();

            _mutex = other._mutex;
            _locked = other._locked;
            other._locked = false;
        }

        return *this;
    }

    /**
     * Returns an awaitable that suspends the calling coroutine unless the lock on the associated mutex is acquired.
     */
    auto async_lock() {
        assert(_mutex);
        assert(!_locked);

        using mutex_awaitable = std::remove_cvref_t<decltype(_mutex->async_lock())>;

        class awaitable {
        public:
            explicit awaitable(self_type &self, mutex_awaitable &&awaitable)
                    : _self(self), _awaitable(std::move(awaitable)) {
            }

            bool await_ready() {
                return _awaitable.await_ready();
            }

            constexpr void await_resume() const noexcept {
                _self._locked = true;
            }

            auto await_suspend(std::coroutine_handle<> continuation) {
                return _awaitable.await_suspend(continuation);
            }

        private:
            self_type &_self;
            mutex_awaitable _awaitable;
        };

        return awaitable(*this, _mutex->async_lock());
    }

    /**
     * Unlocks the associated mutex lock.
     */
    void unlock() {
        if (_locked) {
            _mutex->unlock();
            _locked = false;
        }
    }

    /**
     * Checks whether this instance of unique lock has a lock over the associated mutex.
     */
    bool owns_lock() const noexcept {
        return _locked;
    }

    /**
     * Swaps two unique locks.
     */
    void swap(self_type &other) noexcept {
        std::swap(_mutex, other._mutex);
        std::swap(_locked, other._locked);
    }

private:
    Mutex *_mutex;
    bool _locked;
};

/**
 * Simple asynchronous mutex.
 */
template<class Executor = boost::asio::io_context>
class mutex {
public:
    /**
     * Constructor. Creates an instance of mutex that operates within the specified executor context.
     *
     * @param executor  ASIO executor.
     */
    explicit mutex(Executor &executor)
            : _executor(executor), _locked(false) {
    }

    /**
     * Returns an awaitable which pauses the awaiting coroutine until the mutex is unlocked.
     */
    auto async_lock() {
        class awaitable {
        public:
            explicit awaitable(mutex<Executor> &self)
                    : _self(self) {
            }

            constexpr bool await_ready() const noexcept {
                return false;
            }

            constexpr void await_resume() const noexcept {
            }

            bool await_suspend(std::coroutine_handle<> continuation) {
                {
                    std::lock_guard lock(_self._mutex);
                    if (_self._locked) {
                        _self._continuations.emplace_back(continuation);
                        return true;
                    }

                    _self._locked = true;
                }

                return false;
            }

        private:
            mutex<Executor> &_self;
        };

        return awaitable(*this);
    }

    /**
     * Unlocks the lock. Schedules execution of the pending completion handler if any.
     */
    void unlock() {
        detail::coroutine_holder<> holder;
        {
            std::lock_guard lock(_mutex);

            assert(_locked);

            _locked = false;
            if (!_continuations.empty()) {
                holder = std::move(_continuations.front());
                _continuations.erase(_continuations.begin());
                _locked = true;
            }
        }

        if (holder.valid()) {
            boost::asio::post([holder = std::move(holder)]() mutable {
                holder.release().resume();
            });
        }
    }

private:
    Executor &_executor;
    std::mutex _mutex;
    std::vector<detail::coroutine_holder<>> _continuations;
    bool _locked;
};
} // namespace asio_coro

#endif // ASIO_CORO_EXTENSIONS_MUTEX_HPP
