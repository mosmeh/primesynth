#pragma once
#include <array>
#include "conv.h"
#include "fixedpoint.h"

namespace primasynth {

class Envelope {
public:
    enum class Section {
        Delay,
        Attack,
        Hold,
        Decay,
        Sustain,
        Release,
        Finished
    };

    explicit Envelope(double outputRate) :
        outputRate_(outputRate),
        params_{},
        section_(Section::Delay),
        periodPhase_(0U),
        atten_(1.0),
        value_(1.0) {}

    void setParameter(Section section, double param) {
        if (section == Section::Sustain) {
            params_.at(static_cast<int>(Section::Sustain)) = 0.001 * param;
        } else if (section < Section::Finished) {
            params_.at(static_cast<int>(section)) = timecentToSecond(param);
        } else {
            throw;
        }
    }

    void release() {
        if (section_ < Section::Release) {
            changeSection(Section::Release);
        }
    }

    void finish() {
        changeSection(Section::Finished);
    }

    void update(const FixedPoint& deltaPhase) {
        if (section_ == Section::Finished) {
            return;
        }

        periodPhase_ += deltaPhase;

        auto i = static_cast<int>(section_);
        if (section_ != Section::Sustain && periodPhase_.getReal() >= outputRate_ * params_.at(i)) {
            ++i;
            changeSection(static_cast<Section>(i));
        }

        const double timeSec = periodPhase_.getReal() / outputRate_;
        const double sustain = params_.at(static_cast<int>(Section::Sustain));
        switch (section_) {
        case Section::Delay:
        case Section::Finished:
            atten_ = 1.0;
            break;
        case Section::Attack:
            atten_ = timeSec / params_.at(i);
            break;
        case Section::Hold:
            atten_ = 0;
            break;
        case Section::Decay:
            atten_ = timeSec / params_.at(i);
            if (atten_ > sustain) {
                changeSection(Section::Sustain);
            }
            break;
        case Section::Sustain:
            atten_ = sustain;
            break;
        case Section::Release:
            atten_ += deltaPhase.getReal() / (outputRate_ * params_.at(i));
            if (atten_ > 1.0) {
                changeSection(Section::Finished);
            }
            break;
        }

        if (section_ == Section::Attack) {
            value_ = atten_;
        } else {
            value_ = centibelToRatio(480.0 * atten_);
        }
    }

    double getValue() const {
        return value_;
    }

    bool isFinished() const {
        return section_ == Section::Finished;
    }

private:
    const double outputRate_;
    std::array<double, 6> params_;
    Section section_;
    FixedPoint periodPhase_;
    double atten_, value_;

    void changeSection(Section section) {
        section_ = section;
        periodPhase_ = FixedPoint(0U);
    }
};

}
