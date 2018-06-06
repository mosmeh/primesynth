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
        const auto& preset = findPreset(0, 0);
        for (const auto& channel : channels_) {
            channel->setPreset(preset);
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
        //std::cout << "Note off, key=" << static_cast<int>(msg[1]) << std::endl;
        channel->noteOff(msg[1]);
        break;
    case MIDIMessageStatus::NoteOn:
        /*std::cout << "Note on, key=" << static_cast<int>(msg[1])
            << " velocity=" << static_cast<int>(msg[2]) << std::endl;*/
        channel->noteOn(msg[1], msg[2]);
        break;
    case MIDIMessageStatus::ControlChange:
        /*std::cout << "Control change, controller=" << static_cast<int>(msg[1])
            << " value=" << static_cast<int>(msg[2]) << std::endl;*/
        channel->controlChange(msg[1], msg[2]);
        break;
    case MIDIMessageStatus::ProgramChange: {
        const auto preset = findPreset(channel->isDrumChannel() ? 128 : 0, msg[1]);
        //std::cout << "Program change, program=" << static_cast<int>(msg[1]) << " (" << preset->name << ")" << std::endl;
        channel->setPreset(preset);
        break;
    }
    case MIDIMessageStatus::ChannelPressure:
        //std::cout << "Channel pressure, value=" << static_cast<int>(msg[1]) << std::endl;
        channel->channelPressure(msg[1]);
        break;
    case MIDIMessageStatus::PitchBend: {
        const std::uint16_t value = joinBytes(msg[2], msg[1]);
        //std::cout << "Pitch bend, value=" << value << std::endl;
        channel->pitchBend(value);
        break;
    }
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

std::shared_ptr<const Preset> Synthesizer::findPreset(std::uint8_t bank, std::uint8_t presetNum) const {
    for (const auto& sf : soundFonts_) {
        for (const auto& preset : sf->getPresets()) {
            if (preset->bank == bank && preset->presetNum == presetNum) {
                return preset;
            }
        }
    }
    throw std::runtime_error("preset not found");
}

}
