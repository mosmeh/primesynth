#include "synth.h"
#include "midi.h"

namespace primasynth {

Synthesizer::Synthesizer(double outputRate, std::size_t numChannels) : volume_(1.0) {
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
        defaultDrumPreset_ = findPreset(128, 0);
        for (const auto& channel : channels_) {
            channel->setPreset(channel->isDrumChannel() ? defaultDrumPreset_ : defaultPreset_);
        }
    } else {
        soundFonts_.emplace_back(sf);
    }
    std::cout << "loaded " << sf->getName() << " from " << filename << std::endl;
}

void Synthesizer::processMIDIMessage(DWORD param) {
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
    case MIDIMessageStatus::ProgramChange:
        channel->setPreset(findPreset(channel->isDrumChannel() ? 128 : channel->getBank(), msg[1]));
        break;
    case MIDIMessageStatus::ChannelPressure:
        channel->channelPressure(msg[1]);
        break;
    case MIDIMessageStatus::PitchBend:
        channel->pitchBend(joinBytes(msg[2], msg[1]));
        break;
    }
}

void Synthesizer::setVolume(double volume) {
    volume_ = std::max(0.0, volume);
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
    if (bank == 128) {
        // if drum bank
        if (presetNum != 0 && defaultDrumPreset_) {
            return defaultDrumPreset_;
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
