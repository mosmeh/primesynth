#pragma once
#include "sfspec.h"

namespace primasynth {

class Modulator {
public:
    explicit Modulator(const sf::ModList& param);

    bool isSourceSFController(sf::GeneralController controller) const;
    bool isSourceMIDIController(std::uint8_t controller) const;
    sf::Generator getDestination() const;
    double getValue() const;

    void updateSFController(sf::GeneralController controller, std::int16_t value);
    void updateMIDIController(std::uint8_t controller, std::uint8_t value);

private:
    const sf::ModList param_;
    double source_, amountSource_, value_;

    void calculateValue();
};

}
