#pragma once
#include "soundfont.h"
#include "stereovalue.h"
#include "fixedpoint.h"
#include "modulator.h"
#include "envelope.h"
#include "lfo.h"

namespace primasynth {

class Voice {
public:
    enum class State {
        Playing,
        Sustained,
        Released,
        Finished
    };

    Voice(std::size_t noteID, double outputRate, bool percussion, std::shared_ptr<const Sample> sample,
        const GeneratorSet& generators, const ModulatorParameterSet& modparams, std::uint8_t key, std::uint8_t velocity);

    std::size_t getNoteID() const;
    std::uint8_t getActualKey() const;
    std::int16_t getExclusiveClass() const;
    const State& getStatus() const;
    StereoValue render() const;

    void updateSFController(SFGeneralController controller, std::int16_t value);
    void updateMIDIController(std::uint8_t controller, std::uint8_t value);
    void updateFineTuning(double fineTuning);
    void updateCoarseTuning(double coarseTuning);
    void release(bool sustained);
    void update();

private:
    const std::size_t noteID_;
    const std::uint8_t actualKey_;
    const bool percussion_;
    const std::vector<std::int16_t>& sampleBuffer_;
    GeneratorSet generators_;
    struct {
        SampleMode mode;
        double pitch;
        std::uint32_t start, end, startLoop, endLoop;
    } sample_;
    std::int16_t key_, velocity_;
    std::vector<Modulator> modulators_;
    std::array<double, NUM_GENERATORS> modulated_;
    double fineTuning_, coarseTuning_;
    double deltaPhaseFactor_;
    unsigned int steps_;
    State status_;
    double voicePitch_;
    FixedPoint phase_, deltaPhase_;
    StereoValue volume_;
    Envelope volEnv_, modEnv_;
    LFO vibLFO_, modLFO_;

    double getModulatedGenerator(SFGenerator type) const;
    void updateModulatedParams(SFGenerator destination);
};

}
