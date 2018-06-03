#pragma once
#include "soundfont.h"
#include "stereovalue.h"
#include "modulator.h"
#include "envelope.h"
#include "lfo.h"

namespace primasynth {

class Voice {
public:
    Voice(std::size_t noteID, double outputRate, std::shared_ptr<const Sample> sample,
        const GeneratorSet& generators, const ModulatorParameterSet& modparams, std::uint8_t key, std::uint8_t velocity);

    std::size_t getNoteID() const;
    std::uint8_t getActualKey() const;
    std::int16_t getExclusiveClass() const;
    void update();
    void updateSFController(SFGeneralController controller, std::int16_t value);
    void updateMIDIController(std::uint8_t controller, std::uint8_t value);
    void overrideGenerator(SFGenerator generator, std::int16_t amount);
    StereoValue render() const;
    bool isSounding() const;
    void release();

private:
    const std::size_t noteID_;
    const std::uint8_t actualKey_;
    GeneratorSet generators_;
    struct {
        std::shared_ptr<std::vector<std::int16_t>> buffer;
        SampleMode mode;
        double pitch;
        std::uint32_t start, end, startLoop, endLoop;
    } sample_;
    std::int16_t key_, velocity_;
    std::vector<Modulator> modulators_;
    std::array<double, NUM_GENERATORS> modulations_;
    double deltaPhaseFactor_;
    bool released_;
    double voicePitch_;
    FixedPoint phase_, deltaPhase_;
    StereoValue volume_;
    Envelope volEnv_, modEnv_;
    LFO vibLFO_, modLFO_;

    double getModulatedGenerator(SFGenerator type) const;
    void updateModulatedParams(SFGenerator destination);
};

}
