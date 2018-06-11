#include "voice.h"

namespace primasynth {

static constexpr unsigned int CALC_INTERVAL = 32;

Voice::Voice(std::size_t noteID, double outputRate, bool percussion, std::shared_ptr<const Sample> sample,
    const GeneratorSet& generators, const ModulatorParameterSet& modparams, std::uint8_t key, std::uint8_t velocity) :
    noteID_(noteID),
    percussion_(percussion),
    sampleBuffer_(sample->soundFont->getSampleBuffer()),
    generators_(generators),
    actualKey_(key),
    fineTuning_(0.0),
    coarseTuning_(0.0),
    steps_(0),
    status_(State::Playing),
    phase_(sample->start),
    volume_({1.0, 1.0}),
    volEnv_(outputRate, 1),
    modEnv_(outputRate, CALC_INTERVAL),
    vibLFO_(outputRate, CALC_INTERVAL),
    modLFO_(outputRate, 1) {

    const std::int16_t overriddenSampleKey = generators.getOrDefault(SFGenerator::overridingRootKey);
    sample_.pitch = (overriddenSampleKey > 0 ? overriddenSampleKey : sample->key) - 0.01 * sample->correction;
    sample_.mode = static_cast<SampleMode>(generators.getOrDefault(SFGenerator::sampleModes));
    sample_.start = sample->start
        + 32768 * generators.getOrDefault(SFGenerator::startAddrsCoarseOffset)
        + generators.getOrDefault(SFGenerator::startAddrsOffset);
    sample_.end = sample->end
        + 32768 * generators.getOrDefault(SFGenerator::endAddrsCoarseOffset)
        + generators.getOrDefault(SFGenerator::endAddrsOffset);
    sample_.startLoop = sample->startLoop
        + 32768 * generators.getOrDefault(SFGenerator::startloopAddrsCoarseOffset)
        + generators.getOrDefault(SFGenerator::startloopAddrsOffset);
    sample_.endLoop = sample->endLoop
        + 32768 * generators.getOrDefault(SFGenerator::endloopAddrsCoarseOffset)
        + generators.getOrDefault(SFGenerator::endloopAddrsOffset);

    deltaPhaseFactor_ = 1.0 / keyToHeltz(sample_.pitch) * sample->sampleRate / outputRate;

    for (const auto& mp : modparams.getParameters()) {
        modulators_.emplace_back(mp);
    }

    const std::int16_t genVelocity = generators.getOrDefault(SFGenerator::velocity);
    updateSFController(SFGeneralController::noteOnVelocity, genVelocity > 0 ? genVelocity : velocity);

    const std::int16_t genKey = generators.getOrDefault(SFGenerator::keynum);
    const std::int16_t overriddenKey = genKey > 0 ? genKey : key;
    keyScaling_ = 60 - overriddenKey;
    updateSFController(SFGeneralController::noteOnKeyNumber, overriddenKey);

    for (int i = 0; i < NUM_GENERATORS; ++i) {
        modulated_.at(i) = generators.getOrDefault(static_cast<SFGenerator>(i));
    }
    static const auto INIT_GENERATORS = {
        SFGenerator::pan,
        SFGenerator::delayModLFO,
        SFGenerator::freqModLFO,
        SFGenerator::delayVibLFO,
        SFGenerator::freqVibLFO,
        SFGenerator::delayModEnv,
        SFGenerator::attackModEnv,
        SFGenerator::holdModEnv,
        SFGenerator::decayModEnv,
        SFGenerator::sustainModEnv,
        SFGenerator::releaseModEnv,
        SFGenerator::delayVolEnv,
        SFGenerator::attackVolEnv,
        SFGenerator::holdVolEnv,
        SFGenerator::decayVolEnv,
        SFGenerator::sustainVolEnv,
        SFGenerator::releaseVolEnv,
        SFGenerator::coarseTune
    };
    for (const auto& generator : INIT_GENERATORS) {
        updateModulatedParams(generator);
    }
}

std::size_t Voice::getNoteID() const {
    return noteID_;
}

std::uint8_t Voice::getActualKey() const {
    return actualKey_;
}

std::int16_t Voice::getExclusiveClass() const {
    return generators_.getOrDefault(SFGenerator::exclusiveClass);
}

const Voice::State& Voice::getStatus() const {
    return status_;
}

StereoValue Voice::render() const {
    const std::uint32_t i = phase_.getIntegerPart();
    const double r = phase_.getFractionalPart();
    const double interpolated = (1.0 - r) * sampleBuffer_.at(i) + r * sampleBuffer_.at(i + 1);
    return volEnv_.getValue()
        * centibelToRatio(getModulatedGenerator(SFGenerator::modLfoToVolume) * modLFO_.getValue())
        * volume_ * (interpolated / INT16_MAX);
}

void Voice::updateSFController(SFGeneralController controller, std::int16_t value) {
    for (auto& mod : modulators_) {
        if (mod.isSourceSFController(controller)) {
            mod.updateSFController(controller, value);
            updateModulatedParams(mod.getDestination());
        }
    }
}

void Voice::updateMIDIController(std::uint8_t controller, std::uint8_t value) {
    for (auto& mod : modulators_) {
        if (mod.isSourceMIDIController(controller)) {
            mod.updateMIDIController(controller, value);
            updateModulatedParams(mod.getDestination());
        }
    }
}

void Voice::updateFineTuning(double fineTuning) {
    fineTuning_ = fineTuning;
    updateModulatedParams(SFGenerator::fineTune);
}

void Voice::updateCoarseTuning(double coarseTuning) {
    coarseTuning_ = coarseTuning;
    updateModulatedParams(SFGenerator::coarseTune);
}

void Voice::release(bool sustained) {
     if (percussion_) { 
        return; 
    } 
 
    if (sustained) {
        status_ = State::Sustained;
    } else {
        status_ = State::Released;
        volEnv_.release();
        modEnv_.release();
    }
}

void Voice::update() {
    phase_ += deltaPhase_;

    switch (sample_.mode) {
    case SampleMode::UnLooped:
    case SampleMode::UnUsed:
        if (phase_.getIntegerPart() > sample_.end - 1) {
            status_ = State::Finished;
            return;
        }
        break;
    case SampleMode::Looped:
        if (phase_.getIntegerPart() > sample_.endLoop - 1) {
            phase_ -= FixedPoint(sample_.endLoop - sample_.startLoop);
        }
        break;
    case SampleMode::LoopedWithRemainder:
        if (status_ == State::Released) {
            if (phase_.getIntegerPart() > sample_.end - 1) {
                status_ = State::Finished;
                return;
            }
        } else if (phase_.getIntegerPart() > sample_.endLoop - 1) {
            phase_ -= FixedPoint(sample_.endLoop - sample_.startLoop);
        }
        break;
    }

    modLFO_.update();
    volEnv_.update();

    if (volEnv_.isFinished()) {
        status_ = State::Finished;
        return;
    }

    if (steps_++ % CALC_INTERVAL == 0) {
        vibLFO_.update();
        modEnv_.update();

        deltaPhase_ = FixedPoint(deltaPhaseFactor_ * keyToHeltz(voicePitch_
            + 0.01 * getModulatedGenerator(SFGenerator::modEnvToPitch) * modEnv_.getValue()
            + 0.01 * getModulatedGenerator(SFGenerator::vibLfoToPitch) * vibLFO_.getValue()
            + 0.01 * getModulatedGenerator(SFGenerator::modLfoToPitch) * modLFO_.getValue()));
    }
}

double Voice::getModulatedGenerator(SFGenerator type) const {
    return modulated_.at(static_cast<std::size_t>(type));
}

StereoValue calculatePannedVolume(double pan) {
    if (pan <= -500.0) {
        return {1.0, 0.0};
    } else if (pan >= 500.0) {
        return {0.0, 1.0};
    } else {
        static constexpr double f = 3.141592653589793 / 2000.0;
        return {std::sin(f * (-pan + 500.0)), std::sin(f * (pan + 500.0))};
    }
}

void Voice::updateModulatedParams(SFGenerator destination) {
    double& modulated = modulated_.at(static_cast<std::size_t>(destination));
    modulated = (destination == SFGenerator::initialAttenuation ? 0.4 : 1)
        * generators_.getOrDefault(destination);
    for (auto& mod : modulators_) {
        if (mod.getDestination() == destination) {
            modulated += mod.getValue();
        }
    }
    switch (destination) {
    case SFGenerator::pan:
    case SFGenerator::initialAttenuation: {
        volume_ = centibelToRatio(getModulatedGenerator(SFGenerator::initialAttenuation))
            * calculatePannedVolume(getModulatedGenerator(SFGenerator::pan));
        break;
    }
    case SFGenerator::delayModLFO:
        modLFO_.setDelay(modulated);
        break;
    case SFGenerator::freqModLFO:
        modLFO_.setFrequency(modulated);
        break;
    case SFGenerator::delayVibLFO:
        vibLFO_.setDelay(modulated);
        break;
    case SFGenerator::freqVibLFO:
        vibLFO_.setFrequency(modulated);
        break;
    case SFGenerator::delayModEnv:
        modEnv_.setParameter(Envelope::Section::Delay, modulated);
        break;
    case SFGenerator::attackModEnv:
        modEnv_.setParameter(Envelope::Section::Attack, modulated);
        break;
    case SFGenerator::holdModEnv:
    case SFGenerator::keynumToModEnvHold:
        modEnv_.setParameter(Envelope::Section::Hold,
            getModulatedGenerator(SFGenerator::holdModEnv) + getModulatedGenerator(SFGenerator::keynumToModEnvHold) * keyScaling_);
        break;
    case SFGenerator::decayModEnv:
    case SFGenerator::keynumToModEnvDecay:
        modEnv_.setParameter(Envelope::Section::Decay,
            getModulatedGenerator(SFGenerator::decayModEnv) + getModulatedGenerator(SFGenerator::keynumToModEnvDecay) * keyScaling_);
        break;
    case SFGenerator::sustainModEnv:
        modEnv_.setParameter(Envelope::Section::Sustain, modulated);
        break;
    case SFGenerator::releaseModEnv:
        modEnv_.setParameter(Envelope::Section::Release, modulated);
        break;
    case SFGenerator::delayVolEnv:
        volEnv_.setParameter(Envelope::Section::Delay, modulated);
        break;
    case SFGenerator::attackVolEnv:
        volEnv_.setParameter(Envelope::Section::Attack, modulated);
        break;
    case SFGenerator::holdVolEnv:
    case SFGenerator::keynumToVolEnvHold:
        volEnv_.setParameter(Envelope::Section::Hold,
            getModulatedGenerator(SFGenerator::holdVolEnv) + getModulatedGenerator(SFGenerator::keynumToVolEnvHold) * keyScaling_);
        break;
    case SFGenerator::decayVolEnv:
    case SFGenerator::keynumToVolEnvDecay:
        volEnv_.setParameter(Envelope::Section::Decay,
            getModulatedGenerator(SFGenerator::decayVolEnv) + getModulatedGenerator(SFGenerator::keynumToVolEnvDecay) * keyScaling_);
        break;
    case SFGenerator::sustainVolEnv:
        volEnv_.setParameter(Envelope::Section::Sustain, modulated);
        break;
    case SFGenerator::releaseVolEnv:
        volEnv_.setParameter(Envelope::Section::Release, modulated);
        break;
    case SFGenerator::coarseTune:
    case SFGenerator::fineTune:
    case SFGenerator::scaleTuning:
    case SFGenerator::pitch:
        voicePitch_ = sample_.pitch
            + 0.01 * getModulatedGenerator(SFGenerator::pitch)
            + 0.01 * generators_.getOrDefault(SFGenerator::scaleTuning) * (actualKey_ - sample_.pitch)
            + coarseTuning_ + getModulatedGenerator(SFGenerator::coarseTune)
            + 0.01 * (fineTuning_ + getModulatedGenerator(SFGenerator::fineTune));
        break;
    }
}

}
