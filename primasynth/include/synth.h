#pragma once
#include "channel.h"

namespace primasynth {

class Synthesizer {
public:
    Synthesizer(double outputRate = 44100,
        std::size_t numChannels = 16,
        midi::Standard midiStandard = midi::Standard::GM);

    void loadSoundFont(const std::string& filename);
    void setVolume(double volume);
    void processMIDIMessage(unsigned long param);
    StereoValue render() const;

private:
    const midi::Standard midiStandard_;
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<std::shared_ptr<const SoundFont>> soundFonts_;
    double volume_;
    std::shared_ptr<const Preset> defaultPreset_, defaultPercussionPreset_;

    std::shared_ptr<const Preset> findPreset(std::uint16_t bank, std::uint8_t presetNum) const;
};

}
