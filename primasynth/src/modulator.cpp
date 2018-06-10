#include "modulator.h"

namespace primasynth {

Modulator::Modulator(const sfModList& param) :
    param_(param),
    source_(0.0),
    amountSource_(1.0),
    value_(0.0) {}

bool Modulator::isSourceSFController(SFGeneralController controller) const {
    return (param_.sfModSrcOper.palette == SFControllerPalette::generalController && controller == param_.sfModSrcOper.index.general)
        || (param_.sfModAmtSrcOper.palette == SFControllerPalette::generalController && controller == param_.sfModAmtSrcOper.index.general);
}

bool Modulator::isSourceMIDIController(std::uint8_t controller) const {
    return (param_.sfModSrcOper.palette == SFControllerPalette::midiController && controller == param_.sfModSrcOper.index.midi)
        || (param_.sfModAmtSrcOper.palette == SFControllerPalette::midiController && controller == param_.sfModAmtSrcOper.index.midi);
}

SFGenerator Modulator::getDestination() const {
    return param_.sfModDestOper;
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
        return -4.0 / 9.6 * std::log10(1.0 - x);
    }
}

double convex(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 1 + 4.0 / 9.6 * std::log10(x);
    }
}

double map(double value, SFModulator mod) {
    if (mod.palette == SFControllerPalette::generalController
        && mod.index.general == SFGeneralController::pitchWheel) {
        value /= 1 << 14;
    } else {
        value /= 1 << 7;
    }

    if (mod.type == SFControllerType::switchType) {
        const double off = mod.polarity == SFControllerPolarity::unipolar ? 0.0 : -1.0;
        const double x = mod.direction == SFControllerDirection::increase ? value : 1.0 - value;
        return x >= 0.5 ? 1.0 : off;
    } else if (mod.polarity == SFControllerPolarity::unipolar) {
        const double x = mod.direction == SFControllerDirection::increase ? value : 1.0 - value;
        switch (mod.type) {
        case SFControllerType::linearType:
            return x;
        case SFControllerType::concaveType:
            return concave(x);
        case SFControllerType::convexType:
            return convex(x);
        }
    } else {
        const int dir = mod.direction == SFControllerDirection::increase ? 1 : -1;
        const int sign = value > 0.5 ? 1 : -1;
        const double x = 2.0 * value - 1.0;
        switch (mod.type) {
        case SFControllerType::linearType:
            return dir * x;
        case SFControllerType::concaveType:
            return sign * dir * concave(sign * x);
        case SFControllerType::convexType:
            return sign * dir * convex(sign * x);
        }
    }
    throw std::runtime_error("unknown modulator controller type");
}

void Modulator::updateSFController(SFGeneralController controller, std::int16_t value) {
    if (param_.sfModSrcOper.palette == SFControllerPalette::generalController && controller == param_.sfModSrcOper.index.general) {
        source_ = map(value, param_.sfModSrcOper);
    }
    if (param_.sfModAmtSrcOper.palette == SFControllerPalette::generalController && controller == param_.sfModAmtSrcOper.index.general) {
        amountSource_ = map(value, param_.sfModAmtSrcOper);
    }
    calculateValue();
}

void Modulator::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    if (param_.sfModSrcOper.palette == SFControllerPalette::midiController && controller == param_.sfModSrcOper.index.midi) {
        source_ = map(value, param_.sfModSrcOper);
    }
    if (param_.sfModAmtSrcOper.palette == SFControllerPalette::midiController && controller == param_.sfModAmtSrcOper.index.midi) {
        amountSource_ = map(value, param_.sfModAmtSrcOper);
    }
    calculateValue();
}

double transform(double value, SFTransform transform) {
    switch (transform) {
    case SFTransform::linear:
        return value;
    case SFTransform::absoluteValue:
        return std::abs(value);
    }
    throw std::invalid_argument("unknown transform");
}

void Modulator::calculateValue() {
    value_ = transform(param_.modAmount * source_ * amountSource_, param_.sfModTransOper);
}

}
