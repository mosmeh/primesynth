#include "synth.h"

namespace primasynth {

Synthesizer::Synthesizer(double outputRate, std::size_t numChannels, MIDIStandard midiStandard) :
    volume_(1.0),
    midiStandard_(midiStandard) {

    initializeConversionTables();

    channels_.reserve(numChannels);
    for (std::size_t i = 0; i < numChannels; ++i) {
        channels_.emplace_back(std::make_unique<Channel>(outputRate, i == 9));
    }
}

void Synthesizer::loadSoundFont(const std::string& filename) {
    const auto sf = std::make_shared<SoundFont>(filename);
    if (soundFonts_.empty()) {
        soundFonts_.emplace_back(sf);
        defaultPreset_ = findPreset(0, 0);
        defaultPercussionPreset_ = findPreset(PERCUSSION_BANK, 0);
        for (const auto& channel : channels_) {
            channel->setPreset(channel->isPercussionChannel() ? defaultPercussionPreset_ : defaultPreset_);
        }
    } else {
        soundFonts_.emplace_back(sf);
    }
    std::cout << "loaded " << sf->getName() << " from " << filename << std::endl;
}

void Synthesizer::setVolume(double volume) {
    volume_ = std::max(0.0, volume);
}

void Synthesizer::processMIDIMessage(unsigned long param) {
    const auto msg = reinterpret_cast<std::uint8_t*>(&param);
    const auto& channel = channels_.at(msg[0] & 0xf);
    const auto status = static_cast<MIDIMessageStatus>(msg[0] & 0xf0);
    switch (status) {
    case MIDIMessageStatus::NoteOff:
        channel->noteOff(msg[1]);
        break;
    case MIDIMessageStatus::NoteOn:
        channel->noteOn(msg[1], msg[2]);
        break;
    case MIDIMessageStatus::ControlChange:
        channel->controlChange(msg[1], msg[2]);
        break;
    case MIDIMessageStatus::ProgramChange: {
        const auto midiBank = channel->getBank();
        std::uint16_t sfBank = 0;
        switch (midiStandard_) {
        case MIDIStandard::GS:
            sfBank = midiBank.msb;
            break;
        case MIDIStandard::XG:
            // msb=0:   normal voices
            // msb=127: percussion voices
            // assuming no one uses SFX (msb=64, 126)
            sfBank = midiBank.msb == 127 ? PERCUSSION_BANK : midiBank.lsb;
            break;
        }
        channel->setPreset(findPreset(channel->isPercussionChannel() ? PERCUSSION_BANK : sfBank, msg[1]));
        break;
    }
    case MIDIMessageStatus::ChannelPressure:
        channel->channelPressure(msg[1]);
        break;
    case MIDIMessageStatus::PitchBend:
        channel->pitchBend(joinBytes(msg[2], msg[1]));
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
        for (const auto& preset : sf->getPresets()) {
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
