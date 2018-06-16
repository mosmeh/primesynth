#include "voice.h"

namespace primasynth {

static constexpr unsigned int CALC_INTERVAL = 32;

Voice::Voice(std::size_t noteID, double outputRate, const Sample& sample,
    const GeneratorSet& generators, const ModulatorParameterSet& modparams, std::uint8_t key, std::uint8_t velocity) :
    noteID_(noteID),
    sampleBuffer_(sample.buffer),
    generators_(generators),
    actualKey_(key),
    percussion_(false),
    fineTuning_(0.0),
    coarseTuning_(0.0),
    steps_(0),
    status_(State::Playing),
    phase_(sample.start),
    volume_({1.0, 1.0}),
    volEnv_(outputRate, 1),
    modEnv_(outputRate, CALC_INTERVAL),
    vibLFO_(outputRate, CALC_INTERVAL),
    modLFO_(outputRate, 1) {

    rtSample_.mode = static_cast<SampleMode>(generators.getOrDefault(sf::Generator::sampleModes));
    const std::int16_t overriddenSampleKey = generators.getOrDefault(sf::Generator::overridingRootKey);
    rtSample_.pitch = (overriddenSampleKey > 0 ? overriddenSampleKey : sample.key) - 0.01 * sample.correction;
    rtSample_.start = sample.start
        + 32768 * generators.getOrDefault(sf::Generator::startAddrsCoarseOffset)
        + generators.getOrDefault(sf::Generator::startAddrsOffset);
    rtSample_.end = sample.end
        + 32768 * generators.getOrDefault(sf::Generator::endAddrsCoarseOffset)
        + generators.getOrDefault(sf::Generator::endAddrsOffset);
    rtSample_.startLoop = sample.startLoop
        + 32768 * generators.getOrDefault(sf::Generator::startloopAddrsCoarseOffset)
        + generators.getOrDefault(sf::Generator::startloopAddrsOffset);
    rtSample_.endLoop = sample.endLoop
        + 32768 * generators.getOrDefault(sf::Generator::endloopAddrsCoarseOffset)
        + generators.getOrDefault(sf::Generator::endloopAddrsOffset);

    deltaPhaseFactor_ = 1.0 / conv::keyToHeltz(rtSample_.pitch) * sample.sampleRate / outputRate;

    for (const auto& mp : modparams.getParameters()) {
        modulators_.emplace_back(mp);
    }

    const std::int16_t genVelocity = generators.getOrDefault(sf::Generator::velocity);
    updateSFController(sf::GeneralController::noteOnVelocity, genVelocity > 0 ? genVelocity : velocity);

    const std::int16_t genKey = generators.getOrDefault(sf::Generator::keynum);
    const std::int16_t overriddenKey = genKey > 0 ? genKey : key;
    keyScaling_ = 60 - overriddenKey;
    updateSFController(sf::GeneralController::noteOnKeyNumber, overriddenKey);

    for (int i = 0; i < NUM_GENERATORS; ++i) {
        modulated_.at(i) = generators.getOrDefault(static_cast<sf::Generator>(i));
    }
    static const auto INIT_GENERATORS = {
        sf::Generator::pan,
        sf::Generator::delayModLFO,
        sf::Generator::freqModLFO,
        sf::Generator::delayVibLFO,
        sf::Generator::freqVibLFO,
        sf::Generator::delayModEnv,
        sf::Generator::attackModEnv,
        sf::Generator::holdModEnv,
        sf::Generator::decayModEnv,
        sf::Generator::sustainModEnv,
        sf::Generator::releaseModEnv,
        sf::Generator::delayVolEnv,
        sf::Generator::attackVolEnv,
        sf::Generator::holdVolEnv,
        sf::Generator::decayVolEnv,
        sf::Generator::sustainVolEnv,
        sf::Generator::releaseVolEnv,
        sf::Generator::coarseTune
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
    return generators_.getOrDefault(sf::Generator::exclusiveClass);
}

const Voice::State& Voice::getStatus() const {
    return status_;
}

StereoValue Voice::render() const {
    const std::uint32_t i = phase_.getIntegerPart();
    const double r = phase_.getFractionalPart();
    const double interpolated = (1.0 - r) * sampleBuffer_.at(i) + r * sampleBuffer_.at(i + 1);
    return volEnv_.getValue()
        * conv::attenToAmp(getModulatedGenerator(sf::Generator::modLfoToVolume) * modLFO_.getValue())
        * volume_ * (interpolated / INT16_MAX);
}

void Voice::setPercussion(bool percussion) {
    percussion_ = percussion;
}

void Voice::updateSFController(sf::GeneralController controller, std::int16_t value) {
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
    updateModulatedParams(sf::Generator::fineTune);
}

void Voice::updateCoarseTuning(double coarseTuning) {
    coarseTuning_ = coarseTuning;
    updateModulatedParams(sf::Generator::coarseTune);
}

void Voice::release(bool sustained) {
    if (percussion_) {
        // cf. General MIDI System Level 1 Developer Guidelines Second Revision p.15
        // Response to Note-off on Channel 10 (Percussion)

        // Most of percussion presets sound naturally when they do not respond to note-offs
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

    switch (rtSample_.mode) {
    case SampleMode::UnLooped:
    case SampleMode::UnUsed:
        if (phase_.getIntegerPart() > rtSample_.end - 1) {
            status_ = State::Finished;
            return;
        }
        break;
    case SampleMode::Looped:
        if (phase_.getIntegerPart() > rtSample_.endLoop - 1) {
            phase_ -= FixedPoint(rtSample_.endLoop - rtSample_.startLoop);
        }
        break;
    case SampleMode::LoopedWithRemainder:
        if (status_ == State::Released) {
            if (phase_.getIntegerPart() > rtSample_.end - 1) {
                status_ = State::Finished;
                return;
            }
        } else if (phase_.getIntegerPart() > rtSample_.endLoop - 1) {
            phase_ -= FixedPoint(rtSample_.endLoop - rtSample_.startLoop);
        }
        break;
    default:
        throw std::runtime_error("unknown sample mode");
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

        deltaPhase_ = FixedPoint(deltaPhaseFactor_ * conv::keyToHeltz(voicePitch_
            + 0.01 * getModulatedGenerator(sf::Generator::modEnvToPitch) * modEnv_.getValue()
            + 0.01 * getModulatedGenerator(sf::Generator::vibLfoToPitch) * vibLFO_.getValue()
            + 0.01 * getModulatedGenerator(sf::Generator::modLfoToPitch) * modLFO_.getValue()));
    }
}

double Voice::getModulatedGenerator(sf::Generator type) const {
    return modulated_.at(static_cast<std::size_t>(type));
}

StereoValue calculatePannedVolume(double pan) {
    if (pan <= -500.0) {
        return {1.0, 0.0};
    } else if (pan >= 500.0) {
        return {0.0, 1.0};
    } else {
        static constexpr double FACTOR = 3.141592653589793 / 2000.0;
        return {std::sin(FACTOR * (-pan + 500.0)), std::sin(FACTOR * (pan + 500.0))};
    }
}

void Voice::updateModulatedParams(sf::Generator destination) {
    double& modulated = modulated_.at(static_cast<std::size_t>(destination));
    modulated = (destination == sf::Generator::initialAttenuation ? 0.4 : 1)
        * generators_.getOrDefault(destination);
    for (auto& mod : modulators_) {
        if (mod.getDestination() == destination) {
            modulated += mod.getValue();
        }
    }
    switch (destination) {
    case sf::Generator::pan:
    case sf::Generator::initialAttenuation: {
        volume_ = conv::attenToAmp(getModulatedGenerator(sf::Generator::initialAttenuation))
            * calculatePannedVolume(getModulatedGenerator(sf::Generator::pan));
        break;
    }
    case sf::Generator::delayModLFO:
        modLFO_.setDelay(modulated);
        break;
    case sf::Generator::freqModLFO:
        modLFO_.setFrequency(modulated);
        break;
    case sf::Generator::delayVibLFO:
        vibLFO_.setDelay(modulated);
        break;
    case sf::Generator::freqVibLFO:
        vibLFO_.setFrequency(modulated);
        break;
    case sf::Generator::delayModEnv:
        modEnv_.setParameter(Envelope::Section::Delay, modulated);
        break;
    case sf::Generator::attackModEnv:
        modEnv_.setParameter(Envelope::Section::Attack, modulated);
        break;
    case sf::Generator::holdModEnv:
    case sf::Generator::keynumToModEnvHold:
        modEnv_.setParameter(Envelope::Section::Hold,
            getModulatedGenerator(sf::Generator::holdModEnv) + getModulatedGenerator(sf::Generator::keynumToModEnvHold) * keyScaling_);
        break;
    case sf::Generator::decayModEnv:
    case sf::Generator::keynumToModEnvDecay:
        modEnv_.setParameter(Envelope::Section::Decay,
            getModulatedGenerator(sf::Generator::decayModEnv) + getModulatedGenerator(sf::Generator::keynumToModEnvDecay) * keyScaling_);
        break;
    case sf::Generator::sustainModEnv:
        modEnv_.setParameter(Envelope::Section::Sustain, modulated);
        break;
    case sf::Generator::releaseModEnv:
        modEnv_.setParameter(Envelope::Section::Release, modulated);
        break;
    case sf::Generator::delayVolEnv:
        volEnv_.setParameter(Envelope::Section::Delay, modulated);
        break;
    case sf::Generator::attackVolEnv:
        volEnv_.setParameter(Envelope::Section::Attack, modulated);
        break;
    case sf::Generator::holdVolEnv:
    case sf::Generator::keynumToVolEnvHold:
        volEnv_.setParameter(Envelope::Section::Hold,
            getModulatedGenerator(sf::Generator::holdVolEnv) + getModulatedGenerator(sf::Generator::keynumToVolEnvHold) * keyScaling_);
        break;
    case sf::Generator::decayVolEnv:
    case sf::Generator::keynumToVolEnvDecay:
        volEnv_.setParameter(Envelope::Section::Decay,
            getModulatedGenerator(sf::Generator::decayVolEnv) + getModulatedGenerator(sf::Generator::keynumToVolEnvDecay) * keyScaling_);
        break;
    case sf::Generator::sustainVolEnv:
        volEnv_.setParameter(Envelope::Section::Sustain, modulated);
        break;
    case sf::Generator::releaseVolEnv:
        volEnv_.setParameter(Envelope::Section::Release, modulated);
        break;
    case sf::Generator::coarseTune:
    case sf::Generator::fineTune:
    case sf::Generator::scaleTuning:
    case sf::Generator::pitch:
        voicePitch_ = rtSample_.pitch
            + 0.01 * getModulatedGenerator(sf::Generator::pitch)
            + 0.01 * generators_.getOrDefault(sf::Generator::scaleTuning) * (actualKey_ - rtSample_.pitch)
            + coarseTuning_ + getModulatedGenerator(sf::Generator::coarseTune)
            + 0.01 * (fineTuning_ + getModulatedGenerator(sf::Generator::fineTune));
        break;
    }
}

}
