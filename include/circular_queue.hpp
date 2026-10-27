#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

// Queue FIFO tự cài đặt bằng mảng vòng; vector chỉ quản lý vùng nhớ của mảng.
template <typename T>
class CircularQueue {
public:
    explicit CircularQueue(std::size_t initialCapacity = 8)
        : data_(initialCapacity == 0 ? 1 : initialCapacity) {}

    CircularQueue(const CircularQueue&) = delete;
    CircularQueue& operator=(const CircularQueue&) = delete;
    CircularQueue(CircularQueue&&) noexcept = default;
    CircularQueue& operator=(CircularQueue&&) noexcept = default;

    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }
    [[nodiscard]] std::size_t size() const noexcept { return count_; }

    // Thêm cuối: vị trí cuối = (đầu + số phần tử) % kích thước mảng.
    void push(T value) {
        if (count_ == data_.size()) grow();
        std::size_t rear = (front_ + count_) % data_.size();
        data_[rear] = std::move(value);
        ++count_;
    }

    // Lấy phần tử ở đầu, sau đó chuyển đầu sang vị trí kế tiếp.
    T pop() {
        if (empty()) throw std::out_of_range("Khong the lay phan tu tu Queue rong.");
        T value = std::move(data_[front_]);
        front_ = (front_ + 1) % data_.size();
        --count_;
        return value;
    }

    T& front() {
        if (empty()) throw std::out_of_range("Queue rong.");
        return data_[front_];
    }

    const T& front() const {
        if (empty()) throw std::out_of_range("Queue rong.");
        return data_[front_];
    }

private:
    // Khi đầy, chuyển các phần tử theo thứ tự FIFO sang mảng lớn gấp đôi.
    void grow() {
        std::vector<T> larger(data_.size() * 2);
        for (std::size_t i = 0; i < count_; ++i) {
            larger[i] = std::move(data_[(front_ + i) % data_.size()]);
        }
        data_ = std::move(larger);
        front_ = 0;
    }

    std::vector<T> data_;       // Mảng lưu phần tử.
    std::size_t front_ = 0;     // Vị trí phần tử đầu tiên.
    std::size_t count_ = 0;     // Số phần tử đang chờ.
};