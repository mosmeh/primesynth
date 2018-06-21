#include "envelope.h"
#include "conversion.h"

namespace primasynth {

Envelope::Envelope(double outputRate, unsigned int interval) :
    effectiveOutputRate_(outputRate / interval),
    params_(),
    section_(Section::Delay),
    sectionSteps_(0),
    atten_(1.0),
    value_(1.0) {}

Envelope::Section Envelope::getSection() const {
    return section_;
}

double Envelope::getValue() const {
    return value_;
}

double Envelope::getAtten() const {
    return atten_;
}

bool Envelope::isFinished() const {
    return section_ == Section::Finished;
}

void Envelope::setParameter(Section section, double param) {
    if (section == Section::Sustain) {
        params_.at(static_cast<std::size_t>(Section::Sustain)) = 0.001 * param;
    } else if (section < Section::Finished) {
        params_.at(static_cast<std::size_t>(section)) = effectiveOutputRate_ * conv::timecentToSecond(param);
    } else {
        throw std::invalid_argument("unknown section");
    }
}

void Envelope::release() {
    if (section_ < Section::Release) {
        changeSection(Section::Release);
    }
}

void Envelope::update() {
    if (section_ == Section::Finished) {
        return;
    }

    ++sectionSteps_;

    auto i = static_cast<std::size_t>(section_);
    while (section_ < Section::Finished && section_ != Section::Sustain && sectionSteps_ >= params_.at(i)) {
        changeSection(static_cast<Section>(++i));
    }

    const double& sustain = params_.at(static_cast<std::size_t>(Section::Sustain));
    switch (section_) {
    case Section::Delay:
    case Section::Finished:
        atten_ = 1.0;
        value_ = 0.0;
        return;
    case Section::Attack:
        atten_ = 1.0 - sectionSteps_ / params_.at(i);
        value_ = 1.0 - atten_;
        return;
    case Section::Hold:
        atten_ = 0.0;
        value_ = 1.0;
        return;
    case Section::Decay:
        atten_ = sectionSteps_ / params_.at(i);
        if (atten_ >= sustain) {
            atten_ = sustain;
            changeSection(Section::Sustain);
        }
        break;
    case Section::Sustain:
        atten_ = sustain;
        break;
    case Section::Release:
        atten_ += 1.0 / params_.at(i);
        if (atten_ >= 1.0) {
            atten_ = 1.0;
            changeSection(Section::Finished);
        }
        break;
    }

    value_ = conv::attenToAmp(960.0 * atten_);
}

void Envelope::changeSection(Section section) {
    section_ = section;
    sectionSteps_ = 0;
}

}
