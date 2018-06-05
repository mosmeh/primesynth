#pragma once
#include "conv.h"
#include "fixedpoint.h"

namespace primasynth {

class LFO {
public:
    explicit LFO(double outputRate) :
        outputRate_(outputRate),
        steps_(0),
        delay_(0.0),
        delta_(0.0),
        value_(0.0),
        up_(true) {}

    void setDelay(double delay) {
        delay_ = outputRate_ * timecentToSecond(delay);
    }

    void setFrequency(double freq) {
        delta_ = 4.0 * absoluteCentToHeltz(freq) / outputRate_;
    }

    void update() {
        ++steps_;
        if (steps_ < delay_) {
            return;
        }
        if (up_) {
            value_ += delta_;
            if (value_ > 1.0) {
                value_ = 2.0 - value_;
                up_ = false;
            }
        } else {
            value_ -= delta_;
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
    unsigned int steps_;
    double delay_, delta_, value_;
    bool up_;
};

}
