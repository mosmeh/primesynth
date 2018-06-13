#include "synthesizer.h"

namespace primasynth {

Synthesizer::Synthesizer(double outputRate, std::size_t numChannels, midi::Standard midiStandard, bool standardFixed) :
    volume_(1.0),
    midiStandard_(midiStandard),
    initialMIDIStandard_(midiStandard),
    standardFixed_(standardFixed) {

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

void Synthesizer::processShortMessage(unsigned long param) {
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
        case midi::Standard::GM:
            break;
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

template <std::size_t N>
bool matchSysEx(const char* data, std::size_t length, const std::array<unsigned char, N>& sysEx) {
    if (length != N) {
        return false;
    }

    for (std::size_t i = 0; i < N; ++i) {
        if (i == 2) {
            continue;
        } else if (data[i] != static_cast<char>(sysEx.at(i))) {
            return false;
        }
    }
    return true;
}

void Synthesizer::processSysEx(const char* data, std::size_t length) {
    static constexpr std::array<unsigned char, 6>  GM_SYSTEM_ON        = {0xf0, 0x7e, 0, 0x09, 0x01, 0xf7};
    static constexpr std::array<unsigned char, 6>  GM_SYSTEM_OFF       = {0xf0, 0x7e, 0, 0x09, 0x02, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_RESET            = {0xf0, 0x41, 0, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_SYSTEM_MODE_SET1 = {0xf0, 0x41, 0, 0x42, 0x12, 0x00, 0x00, 0x7f, 0x00, 0x01, 0xf7};
    static constexpr std::array<unsigned char, 11> GS_SYSTEM_MODE_SET2 = {0xf0, 0x41, 0, 0x42, 0x12, 0x00, 0x00, 0x7f, 0x01, 0x00, 0xf7};
    static constexpr std::array<unsigned char, 9>  XG_SYSTEM_ON        = {0xf0, 0x43, 0, 0x4c, 0x00, 0x00, 0x7e, 0x00, 0xf7};

    if (standardFixed_) {
        return;
    }
    if (matchSysEx(data, length, GM_SYSTEM_ON)) {
        midiStandard_ = midi::Standard::GM;
    } else if (matchSysEx(data, length, GM_SYSTEM_OFF)) {
        midiStandard_ = initialMIDIStandard_;
    } else if (matchSysEx(data, length, GS_RESET) || matchSysEx(data, length, GS_SYSTEM_MODE_SET1) || matchSysEx(data, length, GS_SYSTEM_MODE_SET2)) {
        midiStandard_ = midi::Standard::GS;
    } else if (matchSysEx(data, length, XG_SYSTEM_ON)) {
        midiStandard_ = midi::Standard::XG;
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