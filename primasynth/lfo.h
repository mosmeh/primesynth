#pragma once
#include "conv.h"
#include "fixedpoint.h"

namespace primasynth {

class LFO {
public:
    explicit LFO(double outputRate) :
        outputRate_(outputRate),
        delay_(0.0),
        delta_(0.0),
        value_(0.0),
        up_(true) {}

    void setDelay(double delay) {
        delay_ = timecentToSecond(delay);
    }

    void setFrequency(double freq) {
        delta_ = 4.0 * absoluteCentToHeltz(freq) / outputRate_;
    }

    void update(const FixedPoint& deltaPhase) {
        phase_ += deltaPhase;
        if (phase_.getReal() / outputRate_ < delay_) {
            return;
        }
        const auto d = delta_ * deltaPhase.getReal();
        if (up_) {
            value_ += d;
            if (value_ > 1.0) {
                value_ = 2.0 - value_;
                up_ = false;
            }
        } else {
            value_ -= d;
            if (value_ < -1.0) {
                value_ = -2.0 - value_;
                up_ = true;
            }
        }
    }

    double getValue() const {
        return value_;
    }

private:
    const double outputRate_;
    FixedPoint phase_;
    double delay_, delta_, value_;
    bool up_;
};

}
