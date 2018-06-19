#include "modulator.h"
#include <stdexcept>
#include "conversion.h"

namespace primasynth {

Modulator::Modulator(const sf::ModList& param) :
    param_(param),
    source_(0.0),
    amountSource_(1.0),
    value_(0.0) {}

bool Modulator::isSourceSFController(sf::GeneralController controller) const {
    return (param_.sfModSrcOper.palette == sf::ControllerPalette::generalController && controller == param_.sfModSrcOper.index.general)
        || (param_.sfModAmtSrcOper.palette == sf::ControllerPalette::generalController && controller == param_.sfModAmtSrcOper.index.general);
}

bool Modulator::isSourceMIDIController(std::uint8_t controller) const {
    return (param_.sfModSrcOper.palette == sf::ControllerPalette::midiController && controller == param_.sfModSrcOper.index.midi)
        || (param_.sfModAmtSrcOper.palette == sf::ControllerPalette::midiController && controller == param_.sfModAmtSrcOper.index.midi);
}

sf::Generator Modulator::getDestination() const {
    return param_.sfModDestOper;
}

std::int16_t Modulator::getAmount() const {
    return param_.modAmount;
}

bool Modulator::isAlwaysNonNegative() const {
    if (param_.sfModTransOper == sf::Transform::absoluteValue
        || param_.modAmount == 0) {

        return true;
    }

    if (param_.modAmount > 0) {
        const bool noSrc = param_.sfModSrcOper.palette == sf::ControllerPalette::generalController
            && param_.sfModSrcOper.index.general == sf::GeneralController::noController;
        const bool uniSrc = param_.sfModSrcOper.polarity == sf::ControllerPolarity::unipolar;
        const bool noAmt = param_.sfModAmtSrcOper.palette == sf::ControllerPalette::generalController
            && param_.sfModAmtSrcOper.index.general == sf::GeneralController::noController;
        const bool uniAmt = param_.sfModAmtSrcOper.polarity == sf::ControllerPolarity::unipolar;

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
        return 2.0 * conv::ampToNormedAtten(1.0 - x);
    }
}

double convex(double x) {
    if (x <= 0.0) {
        return 0.0;
    } else if (x >= 1.0) {
        return 1.0;
    } else {
        return 1 - 2.0 * conv::ampToNormedAtten(x);
    }
}

double map(double value, sf::Modulator mod) {
    if (mod.palette == sf::ControllerPalette::generalController
        && mod.index.general == sf::GeneralController::pitchWheel) {
        value /= 1 << 14;
    } else {
        value /= 1 << 7;
    }

    if (mod.type == sf::ControllerType::switchType) {
        const double off = mod.polarity == sf::ControllerPolarity::unipolar ? 0.0 : -1.0;
        const double x = mod.direction == sf::ControllerDirection::increase ? value : 1.0 - value;
        return x >= 0.5 ? 1.0 : off;
    } else if (mod.polarity == sf::ControllerPolarity::unipolar) {
        const double x = mod.direction == sf::ControllerDirection::increase ? value : 1.0 - value;
        switch (mod.type) {
        case sf::ControllerType::linearType:
            return x;
        case sf::ControllerType::concaveType:
            return concave(x);
        case sf::ControllerType::convexType:
            return convex(x);
        }
    } else {
        const int dir = mod.direction == sf::ControllerDirection::increase ? 1 : -1;
        const int sign = value > 0.5 ? 1 : -1;
        const double x = 2.0 * value - 1.0;
        switch (mod.type) {
        case sf::ControllerType::linearType:
            return dir * x;
        case sf::ControllerType::concaveType:
            return sign * dir * concave(sign * x);
        case sf::ControllerType::convexType:
            return sign * dir * convex(sign * x);
        }
    }
    throw std::runtime_error("unknown modulator controller type");
}

void Modulator::updateSFController(sf::GeneralController controller, std::int16_t value) {
    if (param_.sfModSrcOper.palette == sf::ControllerPalette::generalController && controller == param_.sfModSrcOper.index.general) {
        source_ = map(value, param_.sfModSrcOper);
    }
    if (param_.sfModAmtSrcOper.palette == sf::ControllerPalette::generalController && controller == param_.sfModAmtSrcOper.index.general) {
        amountSource_ = map(value, param_.sfModAmtSrcOper);
    }
    calculateValue();
}

void Modulator::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    if (param_.sfModSrcOper.palette == sf::ControllerPalette::midiController && controller == param_.sfModSrcOper.index.midi) {
        source_ = map(value, param_.sfModSrcOper);
    }
    if (param_.sfModAmtSrcOper.palette == sf::ControllerPalette::midiController && controller == param_.sfModAmtSrcOper.index.midi) {
        amountSource_ = map(value, param_.sfModAmtSrcOper);
    }
    calculateValue();
}

double transform(double value, sf::Transform transform) {
    switch (transform) {
    case sf::Transform::linear:
        return value;
    case sf::Transform::absoluteValue:
        return std::abs(value);
    }
    throw std::invalid_argument("unknown transform");
}

void Modulator::calculateValue() {
    value_ = transform(param_.modAmount * source_ * amountSource_, param_.sfModTransOper);
}

}
