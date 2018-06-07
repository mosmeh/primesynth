#pragma once

namespace primasynth {

class RingBuffer {
public:
    explicit RingBuffer(std::size_t size) :
        size_(size),
        data_(size) {}

    void push(float t) {
        data_.at(mask(end_++)) = t;
    }

    float shift() {
        return data_.at(mask(start_++));
    }

    void clear() {
        start_ = end_;
    }

    bool empty() const {
        return start_ >= end_;
    }

    bool full() const {
        return end_ - start_ >= size_;
    }

    int size() const {
        return end_ - start_;
    }

    int capacity() const {
        return size_ - size();
    }

private:
    std::size_t size_;
    std::vector<float> data_;
    std::size_t start_ = 0, end_ = 0;

    std::size_t mask(std::size_t i) const {
        return i & (size_ - 1);
    }
};

}
