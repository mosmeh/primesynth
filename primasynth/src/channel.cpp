#include "channel.h"

namespace primasynth {

Channel::Channel(double outputRate, bool percussion) :
    outputRate_(outputRate),
    percussion_(percussion),
    controllers_(),
    dataEntryMode_(DataEntryMode::RPN),
    pitchBend_(1 << 13),
    channelPressure_(0),
    pitchBendSensitivity_(2),
    fineTuning_(0.0),
    coarseTuning_(0.0),
    currentNoteID_(0) {

    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Volume)) = 100;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Pan)) = 64;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::Expression)) = 127;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNLSB)) = 127;
    controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNMSB)) = 127;
    voices_.reserve(128);
}

bool Channel::isPercussionChannel() const {
    return percussion_;
}

Bank Channel::getBank() const {
    return {
        controllers_.at(static_cast<std::size_t>(MIDIControlChange::BankSelectMSB)),
        controllers_.at(static_cast<std::size_t>(MIDIControlChange::BankSelectLSB))
    };
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
                        currentNoteID_, outputRate_, preset_->bank == PERCUSSION_BANK, sample, generators, modparams, key, velocity));
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
            voice->release(controllers_.at(static_cast<std::size_t>(MIDIControlChange::Sustain)) >= 64);
        }
    }
}

void Channel::controlChange(std::uint8_t controller, std::uint8_t value) {
    controllers_.at(controller) = value;

    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    switch (static_cast<MIDIControlChange>(controller)) {
    case MIDIControlChange::DataEntryMSB:
        if (dataEntryMode_ == DataEntryMode::RPN) {
            const std::uint16_t rpn = joinBytes(
                controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNMSB)),
                controllers_.at(static_cast<std::size_t>(MIDIControlChange::RPNLSB)));
            const auto data = static_cast<std::int32_t>(joinBytes(
                value, controllers_.at(static_cast<std::size_t>(MIDIControlChange::DataEntryLSB))));

            switch (static_cast<MIDIRPN>(rpn)) {
            case MIDIRPN::PitchBendSensitivity:
                pitchBendSensitivity_ = value;
                for (const auto& voice : voices_) {
                    voice->updateSFController(SFGeneralController::pitchWheelSensitivity, value);
                }
                break;
            case MIDIRPN::FineTuning: {
                fineTuning_ = (data - 8192) / 81.92;
                for (const auto& voice : voices_) {
                    voice->updateFineTuning(fineTuning_);
                }
                break;
            }
            case MIDIRPN::CoarseTuning: {
                coarseTuning_ = static_cast<std::int16_t>(value) - 64;
                for (const auto& voice : voices_) {
                    voice->updateCoarseTuning(coarseTuning_);
                }
                break;
            }
            }
        }
        break;
    case MIDIControlChange::Sustain:
        for (const auto& voice : voices_) {
            if (voice->getStatus() == Voice::State::Sustained) {
                voice->release(false);
            }
        }
        break;
    case MIDIControlChange::NRPNMSB:
    case MIDIControlChange::NRPNLSB:
        dataEntryMode_ = DataEntryMode::NRPN;
        break;
    case MIDIControlChange::RPNMSB:
    case MIDIControlChange::RPNLSB:
        dataEntryMode_ = DataEntryMode::RPN;
        break;
    case MIDIControlChange::AllSoundOff:
        voices_.clear();
        break;
    case MIDIControlChange::ResetAllControllers:
        // General MIDI System Level 1 Developer Guidelines Second Revision p.5
        // Response to "Reset All Controllers" Message
        pitchBend_ = 1 << 13;
        channelPressure_ = 0;
        for (const auto& voice : voices_) {
            voice->updateSFController(SFGeneralController::pitchWheel, pitchBend_);
            voice->updateSFController(SFGeneralController::channelPressure, channelPressure_);
        }
        for (std::uint8_t i = 1; i < 122; ++i) {
            if ((91 <= i && i <= 95) || (70 <= i && i <= 79)) {
                continue;
            }
            switch (static_cast<MIDIControlChange>(i)) {
            case MIDIControlChange::Volume:
            case MIDIControlChange::Pan:
            case MIDIControlChange::BankSelectLSB:
            case MIDIControlChange::AllSoundOff:
                break;
            case MIDIControlChange::Expression:
            case MIDIControlChange::RPNLSB:
            case MIDIControlChange::RPNMSB:
                controllers_.at(i) = 127;
                for (const auto& voice : voices_) {
                    voice->updateMIDIController(i, 127);
                }
                break;
            default:
                controllers_.at(i) = 0;
                for (const auto& voice : voices_) {
                    voice->updateMIDIController(i, 0);
                }
                break;
            }
        }
        break;
    case MIDIControlChange::AllNotesOff:
        for (const auto& voice : voices_) {
            voice->release(false);
        }
        break;
    default:
        for (const auto& voice : voices_) {
            voice->updateMIDIController(controller, value);
        }
        break;
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
        if (voice->getStatus() != Voice::State::Finished) {
            voice->update();
        }
    }
}

StereoValue Channel::render() {
    std::lock_guard<std::mutex> lockGuard(voiceMutex_);
    StereoValue sum;
    for (const auto& voice : voices_) {
        if (voice->getStatus() != Voice::State::Finished) {
            sum += voice->render();
        }
    }
    return sum;
}

void Channel::addVoice(std::unique_ptr<Voice> voice) {
    voice->updateSFController(SFGeneralController::pitchWheel, pitchBend_);
    voice->updateSFController(SFGeneralController::channelPressure, channelPressure_);
    voice->updateSFController(SFGeneralController::pitchWheelSensitivity, pitchBendSensitivity_);
    voice->updateFineTuning(fineTuning_);
    voice->updateCoarseTuning(coarseTuning_);
    for (std::uint8_t i = 0; i < NUM_CONTROLLERS; ++i) {
        voice->updateMIDIController(i, controllers_.at(i));
    }

    const auto exclusiveClass = voice->getExclusiveClass();
    if (exclusiveClass != 0) {
        for (const auto& v : voices_) {
            if (v->getNoteID() != currentNoteID_ && v->getExclusiveClass() == exclusiveClass) {
                v->release(false);
            }
        }
    }

    for (auto& v : voices_) {
        if (v->getStatus() == Voice::State::Finished) {
            v = std::move(voice);
            return;
        }
    }
    voices_.emplace_back(std::move(voice));
}

}
