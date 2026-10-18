#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

// Deque vòng động hỗ trợ thêm/xóa ở cả hai đầu.
template <typename T>
class Deque {
public:
    explicit Deque(std::size_t initialCapacity = 8)
        : data_(std::make_unique<T[]>(initialCapacity == 0 ? 1 : initialCapacity)),
          capacity_(initialCapacity == 0 ? 1 : initialCapacity) {}

    Deque(const Deque&) = delete;
    Deque& operator=(const Deque&) = delete;
    Deque(Deque&&) noexcept = default;
    Deque& operator=(Deque&&) noexcept = default;

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    void push_back(const T& value) { ensureCapacity(); data_[(head_ + size_) % capacity_] = value; ++size_; }
    void push_back(T&& value) { ensureCapacity(); data_[(head_ + size_) % capacity_] = std::move(value); ++size_; }

    void push_front(const T& value) {
        ensureCapacity();
        head_ = (head_ + capacity_ - 1) % capacity_;
        data_[head_] = value;
        ++size_;
    }
    void push_front(T&& value) {
        ensureCapacity();
        head_ = (head_ + capacity_ - 1) % capacity_;
        data_[head_] = std::move(value);
        ++size_;
    }

    T pop_front() {
        requireNotEmpty();
        T value = std::move(data_[head_]);
        head_ = (head_ + 1) % capacity_;
        --size_;
        if (size_ == 0) head_ = 0;
        return value;
    }
    T pop_back() {
        requireNotEmpty();
        const auto index = (head_ + size_ - 1) % capacity_;
        T value = std::move(data_[index]);
        --size_;
        if (size_ == 0) head_ = 0;
        return value;
    }

    T& front() { requireNotEmpty(); return data_[head_]; }
    const T& front() const { requireNotEmpty(); return data_[head_]; }
    T& back() { requireNotEmpty(); return data_[(head_ + size_ - 1) % capacity_]; }
    const T& back() const { requireNotEmpty(); return data_[(head_ + size_ - 1) % capacity_]; }

    const T& at(std::size_t index) const {
        if (index >= size_) throw std::out_of_range("Chi so Deque vuot pham vi.");
        return data_[(head_ + index) % capacity_];
    }

private:
    void requireNotEmpty() const { if (empty()) throw std::out_of_range("Deque rong."); }
    void ensureCapacity() {
        if (size_ < capacity_) return;
        const std::size_t newCapacity = capacity_ * 2;
        auto expanded = std::make_unique<T[]>(newCapacity);
        for (std::size_t i = 0; i < size_; ++i)
            expanded[i] = std::move(data_[(head_ + i) % capacity_]);
        data_ = std::move(expanded);
        capacity_ = newCapacity;
        head_ = 0;
    }

    std::unique_ptr<T[]> data_;
    std::size_t capacity_;
    std::size_t size_ = 0;
    std::size_t head_ = 0;
};
