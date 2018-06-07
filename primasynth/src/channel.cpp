#include "channel.h"

namespace primasynth {

Channel::Channel(double outputRate, bool drum) :
    outputRate_(outputRate),
    drum_(drum),
    controllers_{},
    pitchBend_(1 << 13),
    channelPressure_(0),
    currentNoteID_(0) {

    controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNMSB)) = 127;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNLSB)) = 127;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Volume)) = 100;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Pan)) = 64;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Expression)) = 127;
    voices_.reserve(128);
}

void Channel::noteOn(std::uint8_t key, std::uint8_t velocity) {
    if (velocity == 0) {
        noteOff(key);
        return;
    }

    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    const auto& soundFont = preset_->soundFont;
    for (const Zone& presetZone : preset_->zones) {
        if (presetZone.isInRange(key, velocity)) {
            const std::int16_t instID = presetZone.generators.getOrDefault(SFGenerator::instrument);
            const auto& inst = soundFont->getInstruments().at(instID);
            for (const Zone& instZone : inst->zones) {
                if (instZone.isInRange(key, velocity)) {
                    const std::int16_t sampleID = instZone.generators.getOrDefault(SFGenerator::sampleID);
                    const auto& sample = soundFont->getSamples().at(sampleID);

                    auto generators = instZone.generators;
                    generators.mergeAndAdd(presetZone.generators);

                    auto modparams = instZone.modulatorParameters;
                    modparams.mergeAndAdd(presetZone.modulatorParameters);
                    modparams.merge(ModulatorParameterSet::getDefaultParameters());

                    addVoice(std::make_unique<Voice>(
                        currentNoteID_, outputRate_, sample, generators, modparams, key, velocity),
                        generators.getOrDefault(SFGenerator::exclusiveClass));
                }
            }
        }
    }
    ++currentNoteID_;
}

void Channel::noteOff(std::uint8_t key) {
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    for (const auto& voice : voices_) {
        if (voice->getActualKey() == key) {
            voice->release();
        }
    }
}

void Channel::controlChange(std::uint8_t controller, std::uint8_t value) {
    controllers_.at(controller) = value;

    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    switch (static_cast<MIDIControlChange>(controller)) {
    case MIDIControlChange::DataEntryMSB: {
        const std::uint16_t rpn = joinBytes(
            controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNMSB)),
            controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNLSB)));
        const std::uint16_t data = joinBytes(
            value, controllers_.at(static_cast<std::size_t>(MIDIControlChange::DataEntryLSB)));

        for (const auto& voice : voices_) {
            switch (static_cast<MIDIRPN>(rpn)) {
            case MIDIRPN::PitchBendSensitivity:
                voice->updateSFController(SFGeneralController::pitchWheelSensitivity, value);
                break;
            case MIDIRPN::FineTuning:
                voice->overrideGenerator(SFGenerator::fineTune, static_cast<std::int16_t>((value - 8192) / 81.92));
                break;
            case MIDIRPN::CoarseTuning:
                voice->overrideGenerator(SFGenerator::coarseTune, value - 64);
                break;
            }
        }
        break;
    }
    case MIDIControlChange::AllSoundOff:
        voices_.clear();
        break;
    case MIDIControlChange::ResetAllControllers:
        // TODO: Reset All Controllers
        break;
    case MIDIControlChange::AllNotesOff:
        for (const auto& voice : voices_) {
            voice->release();
        }
        break;
    }

    for (const auto& voice : voices_) {
        voice->updateMIDIController(controller, value);
    }
}

void Channel::pitchBend(std::uint16_t value) {
    pitchBend_ = value;
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    for (const auto& voice : voices_) {
        voice->updateSFController(SFGeneralController::pitchWheel, value);
    }
}

void Channel::channelPressure(std::uint8_t value) {
    channelPressure_ = value;
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    for (const auto& voice : voices_) {
        voice->updateSFController(SFGeneralController::channelPressure, value);
    }
}

void Channel::setPreset(const std::shared_ptr<const Preset>& preset) {
    preset_ = preset;
}

void Channel::update() {
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    for (const auto& voice : voices_) {
        if (voice->isSounding()) {
            voice->update();
        }
    }
}

StereoValue Channel::render() {
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    StereoValue sum;
    for (const auto& voice : voices_) {
        if (voice->isSounding()) {
            sum += voice->render();
        }
    }
    return sum;
}

bool Channel::isDrumChannel() const {
    return drum_;
}

void Channel::addVoice(std::unique_ptr<Voice> voice, std::int16_t exclusiveClass) {
    voice->updateSFController(SFGeneralController::pitchWheel, pitchBend_);
    voice->updateSFController(SFGeneralController::channelPressure, channelPressure_);
    for (std::uint8_t i = 0; i < NUM_CONTROLLERS; ++i) {
        voice->updateMIDIController(i, controllers_.at(i));
    }

    if (exclusiveClass != 0) {
        for (const auto& v : voices_) {
            if (currentNoteID_ != v->getNoteID() && exclusiveClass == v->getExclusiveClass()) {
                v->release();
            }
        }
    }

    for (auto& v : voices_) {
        if (!v->isSounding()) {
            v = std::move(voice);
            return;
        }
    }
    voices_.emplace_back(std::move(voice));
}

}
