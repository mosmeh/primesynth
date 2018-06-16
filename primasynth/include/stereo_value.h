#pragma once

namespace primasynth {

struct StereoValue {
    double left, right;

    StereoValue() = delete;

    StereoValue operator+(const StereoValue& b) const;
    StereoValue operator*(double b) const;
    StereoValue operator*(const StereoValue& b) const;
    StereoValue& operator+=(const StereoValue& b);
};

StereoValue operator*(double a, const StereoValue& b);

}
