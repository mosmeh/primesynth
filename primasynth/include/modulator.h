#pragma once
#include "soundfont.h"

namespace primasynth {

class Modulator {
public:
    explicit Modulator(const sfModList& param);

    bool isSourceSFController(SFGeneralController controller) const;
    bool isSourceMIDIController(std::uint8_t controller) const;
    SFGenerator getDestination() const;
    double getValue() const;

    void updateSFController(SFGeneralController controller, std::int16_t value);
    void updateMIDIController(std::uint8_t controller, std::uint8_t value);

private:
    const sfModList param_;
    double source_, amountSource_, value_;

    void calculateValue();
};

}
