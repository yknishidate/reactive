#pragma once

namespace rv {
template <typename T>
class ArrayProxy {
public:
    constexpr ArrayProxy() noexcept : m_count(0), m_ptr(nullptr) {}

    constexpr ArrayProxy(std::nullptr_t) noexcept : m_count(0), m_ptr(nullptr) {}

    ArrayProxy(T const& value) noexcept : m_count(1), m_ptr(&value) {}

    ArrayProxy(uint32_t count, T const* ptr) noexcept : m_count(count), m_ptr(ptr) {}

    template <std::size_t C>
    ArrayProxy(T const (&ptr)[C]) noexcept : m_count(C), m_ptr(ptr) {}

#if __GNUC__ >= 9
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winit-list-lifetime"
#endif

    ArrayProxy(std::initializer_list<T> const& list) noexcept
        : m_count(static_cast<uint32_t>(list.size())), m_ptr(list.begin()) {}

    template <typename B = T, typename std::enable_if<std::is_const<B>::value, int>::type = 0>
    ArrayProxy(std::initializer_list<typename std::remove_const<T>::type> const& list) noexcept
        : m_count(static_cast<uint32_t>(list.size())), m_ptr(list.begin()) {}

#if __GNUC__ >= 9
#pragma GCC diagnostic pop
#endif

    // Any type with a .data() return type implicitly convertible to T*, and a .size() return type
    // implicitly convertible to size_t. The const version can capture temporaries, with lifetime
    // ending at end of statement.
    template <typename V,
              typename std::enable_if<
                  std::is_convertible<decltype(std::declval<V>().data()), T*>::value &&
                  std::is_convertible<decltype(std::declval<V>().size()),
                                      std::size_t>::value>::type* = nullptr>
    ArrayProxy(V const& v) noexcept : m_count(static_cast<uint32_t>(v.size())), m_ptr(v.data()) {}

    template <typename V,
              typename std::enable_if<std::is_convertible<decltype(std::declval<V>().data()),
                                                          T*>::value>::type* = nullptr>
    ArrayProxy(V const& v, uint32_t startIndex, uint32_t count) noexcept
        : m_count(count), m_ptr(v.data() + startIndex) {}

    const T* begin() const noexcept { return m_ptr; }

    const T* end() const noexcept { return m_ptr + m_count; }

    const T& front() const noexcept {
        assert(m_count && m_ptr);
        return *m_ptr;
    }

    const T& back() const noexcept {
        assert(m_count && m_ptr);
        return *(m_ptr + m_count - 1);
    }

    bool empty() const noexcept { return (m_count == 0); }

    uint32_t size() const noexcept { return m_count; }

    T const* data() const noexcept { return m_ptr; }

    bool contains(const T& x) const noexcept {
        for (uint32_t i = 0; i < m_count; i++) {
            if (x == m_ptr[i]) {
                return true;
            }
        }
        return false;
    }

    const T& operator[](uint32_t i) const noexcept {
        assert(0 <= i && i < m_count);
        return m_ptr[i];
    }

private:
    uint32_t m_count;
    const T* m_ptr;
};
}  // namespace rv
