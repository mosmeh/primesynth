#pragma once

namespace primasynth {

class FixedPoint {
public:
    FixedPoint() = delete;

    explicit FixedPoint(std::uint32_t integer) :
        raw_(static_cast<std::uint64_t>(integer) << 32) {}

    explicit FixedPoint(double value) :
        raw_((static_cast<std::uint64_t>(value) << 32)
            | static_cast<std::uint32_t>((value - static_cast<std::uint32_t>(value))
                * (static_cast<double>(UINT32_MAX) + 1.0))) {}

    std::uint64_t getRaw() const {
        return raw_;
    }

    std::uint32_t getIntegerPart() const {
        return raw_ >> 32;
    }

    double getFractionalPart() const {
        return (raw_ & UINT32_MAX) / (static_cast<double>(UINT32_MAX) + 1.0);
    }

    double getReal() const {
        return getIntegerPart() + getFractionalPart();
    }

    std::uint32_t getRoundedInteger() const {
        return ((raw_ + INT32_MAX) + 1) >> 32;
    }

    FixedPoint& operator+=(const FixedPoint& b) {
        raw_ += b.raw_;
        return *this;
    }

    FixedPoint operator+(const FixedPoint& b) const {
        auto x = *this;
        x += b;
        return x;
    }

    FixedPoint& operator-=(const FixedPoint& b) {
        raw_ -= b.raw_;
        return *this;
    }

    FixedPoint operator-(const FixedPoint& b) const {
        auto x = *this;
        x -= b;
        return x;
    }

private:
    std::uint64_t raw_;
};

}

