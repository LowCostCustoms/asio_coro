#ifndef ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HOLDER_HPP
#define ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HOLDER_HPP

#include "coroutine.hpp"

#include <memory>

namespace asio_coro::detail {
/**
 * Helper class used to release coroutine resources automatically.
 */
template<class CoroutineHandle = coroutine_handle<>>
class coroutine_holder {
public:
    using self_type = coroutine_holder<CoroutineHandle>;

    /**
     * Default constructor. Creates a coroutine holder that takes ownership over no coroutine.
     */
    coroutine_holder() noexcept = default;

    /**
     * Constructor. Creates a coroutine holder that takes ownership over specified coroutine.
     */
    coroutine_holder(CoroutineHandle coroutine) noexcept
            : _coroutine(coroutine) {
    }

    /**
     * Move constructor. Transfers ownership over associated coroutine from other coroutine holder to the newly
     * created coroutine holder.
     */
    coroutine_holder(self_type &&other) noexcept
            : _coroutine(other._coroutine) {
        other._coroutine = nullptr;
    }

    /**
     * Destructor. Destroys the associated coroutine if needed.
     */
    ~coroutine_holder() {
        clear();
    }

    /**
     * Move assignment. Destroys the associated coroutine if needed. Transfers ownership over associated coroutine
     * from other coroutine holder to this coroutine holder.
     */
    coroutine_holder &operator=(self_type &&other) noexcept {
        if (this != std::addressof(other)) {
            clear();

            _coroutine = other._coroutine;
            other._coroutine = nullptr;
        }

        return *this;
    }

    /**
     * Releases the ownership over the associated coroutine holder.
     */
    CoroutineHandle release() noexcept {
        const auto result = _coroutine;
        _coroutine = nullptr;

        return result;
    }

    /**
     * Destroys the associated coroutine.
     */
    void clear() noexcept {
        if (_coroutine) {
            _coroutine.destroy();
            _coroutine = nullptr;
        }
    }

    /**
     * Checks whether this coroutine holder takes ownership over any coroutine.
     */
    bool valid() const noexcept {
        return _coroutine != nullptr;
    }

protected:
    CoroutineHandle _coroutine;
};
} // namespace asio_coro::detail

#endif // ASIO_CORO_EXTENSIONS_DETAIL_COROUTINE_HOLDER_HPP
