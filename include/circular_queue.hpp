#pragma once

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

// Queue FIFO dùng mảng vòng động, tự quản lý bộ nhớ, không dùng std::queue.
template <typename T>
class CircularQueue {
public:
    explicit CircularQueue(std::size_t initialCapacity = 8)
        : data_(std::make_unique<T[]>(initialCapacity == 0 ? 1 : initialCapacity)),
          capacity_(initialCapacity == 0 ? 1 : initialCapacity) {}

    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;
    CircularQueue(CircularQueue&&) noexcept = default;
    CircularQueue& operator=(CircularQueue&&) noexcept = default;

    [[nodiscard]] bool empty() const noexcept { return size_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return size_; }

    void push(const T& value) {
        ensureCapacity();
        data_[(head_ + size_) % capacity_] = value;
        ++size_;
    }

    void push(T&& value) {
        ensureCapacity();
        data_[(head_ + size_) % capacity_] = std::move(value);
        ++size_;
    }

    T pop() {
        if (empty()) throw std::out_of_range("Khong the lay phan tu tu Queue rong.");
        T value = std::move(data_[head_]);
        head_ = (head_ + 1) % capacity_;
        --size_;
        if (size_ == 0) head_ = 0;
        return value;
    }

    T& front() {
        if (empty()) throw std::out_of_range("Queue rong.");
        return data_[head_];
    }

    const T& front() const {
        if (empty()) throw std::out_of_range("Queue rong.");
        return data_[head_];
    }

private:
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
