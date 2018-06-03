#pragma once
#include "soundfont.h"
#include "stereovalue.h"
#include "channel.h"

namespace primasynth {

class Synthesizer {
public:
    Synthesizer(double outputRate, std::size_t numChannels);

    void loadSoundFont(const std::string& filename);
    void processMIDIMessage(DWORD param);
    void setVolume(double volume);
    StereoValue render() const;

private:
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<std::shared_ptr<const SoundFont>> soundFonts_;
    double volume_;

    std::shared_ptr<const Preset> findPreset(std::uint8_t bank, std::uint8_t presetNum) const;
};

}
