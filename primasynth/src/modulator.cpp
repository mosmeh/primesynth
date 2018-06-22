#include "conversion.h"
#include "modulator.h"
#include <stdexcept>

namespace primasynth {
Modulator::Modulator(const sf::ModList& param) : param_(param), source_(0.0), amountSource_(1.0), value_(0.0) {}

bool Modulator::isSourceSFController(sf::GeneralController controller) const {
    return (param_.modSrcOper.palette == sf::ControllerPalette::General
            && controller == param_.modSrcOper.index.general)
           || (param_.modAmtSrcOper.palette == sf::ControllerPalette::General
               && controller == param_.modAmtSrcOper.index.general);
}

bool Modulator::isSourceMIDIController(std::uint8_t controller) const {
    return (param_.modSrcOper.palette == sf::ControllerPalette::MIDI && controller == param_.modSrcOper.index.midi)
           || (param_.modAmtSrcOper.palette == sf::ControllerPalette::MIDI
               && controller == param_.modAmtSrcOper.index.midi);
}

sf::Generator Modulator::getDestination() const {
    return param_.modDestOper;
}

std::int16_t Modulator::getAmount() const {
    return param_.modAmount;
}

bool Modulator::isAlwaysNonNegative() const {
    if (param_.modTransOper == sf::Transform::AbsoluteValue || param_.modAmount == 0) {
        return true;
    }

    if (param_.modAmount > 0) {
        const bool noSrc = param_.modSrcOper.palette == sf::ControllerPalette::General
                           && param_.modSrcOper.index.general == sf::GeneralController::NoController;
        const bool uniSrc = param_.modSrcOper.polarity == sf::SourcePolarity::Unipolar;
        const bool noAmt = param_.modAmtSrcOper.palette == sf::ControllerPalette::General
                           && param_.modAmtSrcOper.index.general == sf::GeneralController::NoController;
        const bool uniAmt = param_.modAmtSrcOper.polarity == sf::SourcePolarity::Unipolar;

        if ((uniSrc && uniAmt) || (uniSrc && noAmt) || (noSrc && uniAmt) || (noSrc && noAmt)) {
            return true;
        }
    }

    return false;
}

double Modulator::getValue() const {
    return value_;
}

double concave(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 2.0 * conv::amplitudeToAttenuation(1.0 - x);
    }
}

double convex(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 1.0 - 2.0 * conv::amplitudeToAttenuation(x);
    }
}

double map(double value, sf::Modulator mod) {
    if (mod.palette == sf::ControllerPalette::General && mod.index.general == sf::GeneralController::PitchWheel) {
        value /= 1 << 14;
    } else {
        value /= 1 << 7;
    }

    if (mod.type == sf::SourceType::Switch) {
        const double off = mod.polarity == sf::SourcePolarity::Unipolar ? 0.0 : -1.0;
        const double x = mod.direction == sf::SourceDirection::Positive ? value : 1.0 - value;
        return x >= 0.5 ? 1.0 : off;
    } else if (mod.polarity == sf::SourcePolarity::Unipolar) {
        const double x = mod.direction == sf::SourceDirection::Positive ? value : 1.0 - value;
        switch (mod.type) {
        case sf::SourceType::Linear:
            return x;
        case sf::SourceType::Concave:
            return concave(x);
        case sf::SourceType::Convex:
            return convex(x);
        }
    } else {
        const int dir = mod.direction == sf::SourceDirection::Positive ? 1 : -1;
        const int sign = value > 0.5 ? 1 : -1;
        const double x = 2.0 * value - 1.0;
        switch (mod.type) {
        case sf::SourceType::Linear:
            return dir * x;
        case sf::SourceType::Concave:
            return sign * dir * concave(sign * x);
        case sf::SourceType::Convex:
            return sign * dir * convex(sign * x);
        }
    }
    throw std::runtime_error("unknown modulator controller type");
}

void Modulator::updateSFController(sf::GeneralController controller, std::int16_t value) {
    if (param_.modSrcOper.palette == sf::ControllerPalette::General && controller == param_.modSrcOper.index.general) {
        source_ = map(value, param_.modSrcOper);
    }
    if (param_.modAmtSrcOper.palette == sf::ControllerPalette::General
        && controller == param_.modAmtSrcOper.index.general) {
        amountSource_ = map(value, param_.modAmtSrcOper);
    }
    calculateValue();
}

void Modulator::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    if (param_.modSrcOper.palette == sf::ControllerPalette::MIDI && controller == param_.modSrcOper.index.midi) {
        source_ = map(value, param_.modSrcOper);
    }
    if (param_.modAmtSrcOper.palette == sf::ControllerPalette::MIDI && controller == param_.modAmtSrcOper.index.midi) {
        amountSource_ = map(value, param_.modAmtSrcOper);
    }
    calculateValue();
}

double transform(double value, sf::Transform transform) {
    switch (transform) {
    case sf::Transform::Linear:
        return value;
    case sf::Transform::AbsoluteValue:
        return std::abs(value);
    }
    throw std::invalid_argument("unknown transform");
}

void Modulator::calculateValue() {
    value_ = transform(param_.modAmount * source_ * amountSource_, param_.modTransOper);
}
}
