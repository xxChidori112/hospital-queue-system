#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>
#include <vector>

// Max-heap tự cài đặt. Kiểu T định nghĩa operator< để quyết định độ ưu tiên.
template <typename T>
class PriorityQueue {
public:
    [[nodiscard]] bool empty() const noexcept { return heap_.empty(); }
    [[nodiscard]] std::size_t size() const noexcept { return heap_.size(); }
    const T& top() const {
        if (empty()) throw std::out_of_range("Priority Queue rong.");
        return heap_.front();
    }

    void push(const T& value) { heap_.push_back(value); siftUp(heap_.size() - 1); }
    void push(T&& value) { heap_.push_back(std::move(value)); siftUp(heap_.size() - 1); }

    T pop() {
        if (empty()) throw std::out_of_range("Khong the lay phan tu tu Priority Queue rong.");
        T result = std::move(heap_.front());
        if (heap_.size() == 1) { heap_.pop_back(); return result; }
        heap_.front() = std::move(heap_.back());
        heap_.pop_back();
        siftDown(0);
        return result;
    }

private:
    void siftUp(std::size_t index) {
        while (index > 0) {
            const auto parent = (index - 1) / 2;
            if (!(heap_[parent] < heap_[index])) break;
            std::swap(heap_[parent], heap_[index]);
            index = parent;
        }
    }
    void siftDown(std::size_t index) {
        for (;;) {
            const auto left = index * 2 + 1;
            const auto right = left + 1;
            auto largest = index;
            if (left < heap_.size() && heap_[largest] < heap_[left]) largest = left;
            if (right < heap_.size() && heap_[largest] < heap_[right]) largest = right;
            if (largest == index) return;
            std::swap(heap_[index], heap_[largest]);
            index = largest;
        }
    }

    std::vector<T> heap_;
};
