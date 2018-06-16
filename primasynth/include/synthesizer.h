#pragma once
#include "channel.h"

namespace primasynth {

class Synthesizer {
public:
    Synthesizer(double outputRate = 44100, std::size_t numChannels = 16);

    void loadSoundFont(const std::string& filename);
    void setVolume(double volume);
    void setMIDIStandard(midi::Standard midiStandard, bool fixed = false);
    void processShortMessage(unsigned long param);
    void processSysEx(const char* data, std::size_t length);
    StereoValue render() const;

private:
    midi::Standard midiStd_, defaultMIDIStd_;
    bool stdFixed_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<std::unique_ptr<SoundFont>> soundFonts_;
    double volume_;
    std::shared_ptr<const Preset> defaultPreset_, defaultPercussionPreset_;

    std::shared_ptr<const Preset> findPreset(std::uint16_t bank, std::uint8_t presetNum) const;
};

}
