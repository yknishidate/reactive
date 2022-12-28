#pragma once
#include "Geometry.hpp"

class Mesh;
class Object;

template <typename T>
class ArrayProxy {
public:
    ArrayProxy() : count(0), ptr(nullptr) {}

    ArrayProxy(const T& value) : count(1), ptr(&value) {}

    ArrayProxy(const std::initializer_list<T>& list)
        : count(static_cast<uint32_t>(list.size())), ptr(list.begin()) {}

    template <typename B = T, typename std::enable_if<std::is_const<B>::value, int>::type = 0>
    ArrayProxy(const std::initializer_list<typename std::remove_const<T>::type>& list)
        : count(static_cast<uint32_t>(list.size())), ptr(list.begin()) {}

    template <typename V,
              typename std::enable_if<
                  std::is_convertible<decltype(std::declval<V>().data()), T*>::value &&
                  std::is_convertible<decltype(std::declval<V>().size()),
                                      std::size_t>::value>::type* = nullptr>
    ArrayProxy(const V& v) : count(static_cast<uint32_t>(v.size())), ptr(v.data()) {}

    const T* begin() const { return ptr; }

    const T* end() const { return ptr + count; }

    const T& front() const {
        assert(count && ptr);
        return *ptr;
    }

    const T& back() const {
        assert(count && ptr);
        return *(ptr + count - 1);
    }

    bool empty() const { return (count == 0); }

    uint32_t size() const { return count; }

    T const* data() const { return ptr; }

private:
    uint32_t count;
    T const* ptr;
};

class BottomAccel {
public:
    BottomAccel() = default;
    explicit BottomAccel(const Mesh& mesh,
                         vk::GeometryFlagBitsKHR geometryFlag = vk::GeometryFlagBitsKHR::eOpaque);

    uint64_t getBufferAddress() const { return buffer.getAddress(); }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
};

class TopAccel {
public:
    TopAccel() = default;
    explicit TopAccel(const ArrayProxy<Object>& objects,
                      vk::GeometryFlagBitsKHR geometryFlag = vk::GeometryFlagBitsKHR::eOpaque);

    void rebuild(const ArrayProxy<Object>& objects);

    vk::AccelerationStructureKHR getAccel() const { return *accel; }
    vk::WriteDescriptorSetAccelerationStructureKHR getInfo() const { return {*accel}; }

private:
    vk::UniqueAccelerationStructureKHR accel;
    DeviceBuffer buffer;
    DeviceBuffer instanceBuffer;
    InstancesGeometry geometry;  // TODO: remove
};
