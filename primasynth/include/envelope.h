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

    Envelope(double outputRate, unsigned int interval) :
        outputRate_(outputRate),
        interval_(interval),
        params_(),
        section_(Section::Delay),
        periodSteps_(0),
        atten_(1.0),
        value_(1.0) {}

    void setParameter(Section section, double param) {
        if (section == Section::Sustain) {
            params_.at(static_cast<int>(Section::Sustain)) = 0.001 * param;
        } else if (section < Section::Finished) {
            params_.at(static_cast<int>(section)) = outputRate_ * timecentToSecond(param) / interval_;
        } else {
            throw std::invalid_argument("unknown section");
        }
    }

    void release() {
        if (section_ < Section::Release) {
            changeSection(Section::Release);
        }
    }

    void update() {
        if (section_ == Section::Finished) {
            return;
        }

        ++periodSteps_;

        auto i = static_cast<int>(section_);
        if (section_ != Section::Sustain && periodSteps_ >= params_.at(i)) {
            ++i;
            changeSection(static_cast<Section>(i));
        }

        const double& sustain = params_.at(static_cast<int>(Section::Sustain));
        switch (section_) {
        case Section::Delay:
        case Section::Finished:
            atten_ = 1.0;
            break;
        case Section::Attack:
            atten_ = periodSteps_ / params_.at(i);
            break;
        case Section::Hold:
            atten_ = 0;
            break;
        case Section::Decay:
            atten_ = periodSteps_ / params_.at(i);
            if (atten_ > sustain) {
                changeSection(Section::Sustain);
            }
            break;
        case Section::Sustain:
            atten_ = sustain;
            break;
        case Section::Release:
            atten_ += 1.0 / params_.at(i);
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
    const unsigned int interval_;
    std::array<double, 6> params_;
    Section section_;
    unsigned int periodSteps_;
    double atten_, value_;

    void changeSection(Section section) {
        section_ = section;
        periodSteps_ = 0;
    }
};

}
