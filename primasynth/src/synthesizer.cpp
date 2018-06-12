#include "synthesizer.h"

namespace primasynth {

Synthesizer::Synthesizer(double outputRate, std::size_t numChannels, midi::Standard midiStandard) :
    volume_(1.0),
    midiStandard_(midiStandard) {

    conv::initialize();

    channels_.reserve(numChannels);
    for (std::size_t i = 0; i < numChannels; ++i) {
        channels_.emplace_back(std::make_unique<Channel>(outputRate));
    }
}

void Synthesizer::loadSoundFont(const std::string& filename) {
    soundFonts_.emplace_back(filename);

    if (soundFonts_.size() == 1) {
        defaultPreset_ = findPreset(0, 0);
        defaultPercussionPreset_ = findPreset(PERCUSSION_BANK, 0);
        for (std::size_t i = 0; i < channels_.size(); ++i) {
            channels_.at(i)->setPreset(i == midi::PERCUSSION_CHANNEL ? defaultPercussionPreset_ : defaultPreset_);
        }
    }
}

void Synthesizer::setVolume(double volume) {
    volume_ = std::max(0.0, volume);
}

void Synthesizer::processMIDIMessage(unsigned long param) {
    const auto msg = reinterpret_cast<std::uint8_t*>(&param);
    const std::uint8_t channelNum = msg[0] & 0xf;
    const auto& channel = channels_.at(channelNum);
    const auto status = static_cast<midi::MessageStatus>(msg[0] & 0xf0);
    switch (status) {
    case midi::MessageStatus::NoteOff:
        channel->noteOff(msg[1]);
        break;
    case midi::MessageStatus::NoteOn:
        channel->noteOn(msg[1], msg[2]);
        break;
    case midi::MessageStatus::ControlChange:
        channel->controlChange(msg[1], msg[2]);
        break;
    case midi::MessageStatus::ProgramChange: {
        const auto midiBank = channel->getBank();
        std::uint16_t sfBank = 0;
        switch (midiStandard_) {
        case midi::Standard::GS:
            sfBank = midiBank.msb;
            break;
        case midi::Standard::XG:
            // assuming no one uses XG voices bank MSBs of which overlap normal voices' bank LSBs
            // e.g. SFX voice, MSB=64
            sfBank = midiBank.msb == 127 ? PERCUSSION_BANK : midiBank.lsb;
            break;
        default:
            throw std::runtime_error("unknown MIDI standard");
        }
        channel->setPreset(findPreset(channelNum == midi::PERCUSSION_CHANNEL ? PERCUSSION_BANK : sfBank, msg[1]));
        break;
    }
    case midi::MessageStatus::ChannelPressure:
        channel->channelPressure(msg[1]);
        break;
    case midi::MessageStatus::PitchBend:
        channel->pitchBend(conv::joinBytes(msg[2], msg[1]));
        break;
    }
}

StereoValue Synthesizer::render() const {
    StereoValue sum;
    for (const auto& channel : channels_) {
        channel->update();
        sum += channel->render();
    }
    return volume_ * sum;
}

std::shared_ptr<const Preset> Synthesizer::findPreset(std::uint16_t bank, std::uint8_t presetNum) const {
    for (const auto& sf : soundFonts_) {
        for (const auto& preset : sf.getPresetPtrs()) {
            if (preset->bank == bank && preset->presetNum == presetNum) {
                return preset;
            }
        }
    }

    // fallback
    if (bank == PERCUSSION_BANK) {
        // if percussion bank
        if (presetNum != 0 && defaultPercussionPreset_) {
            return defaultPercussionPreset_;
        } else {
            throw std::runtime_error("failed to find preset 128:0 (GM Percussion)");
        }
    } else if (bank != 0) {
        // fall back to GM bank
        return findPreset(0, presetNum);
    } else if (defaultPreset_) {
        // preset not found even in GM bank, fall back to Piano
        return defaultPreset_;
    } else {
        // Piano not found, there is no more fallback
        throw std::runtime_error("failed to find preset 0:0 (GM Acoustic Grand Piano)");
    }
}

}
