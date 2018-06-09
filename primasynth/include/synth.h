#pragma once
#include "soundfont.h"
#include "stereovalue.h"
#include "channel.h"

namespace primasynth {

class Synthesizer {
public:
    Synthesizer(double outputRate = 44100,
        std::size_t numChannels = 16,
        MIDIStandard midiStandard = MIDIStandard::GM);

    void loadSoundFont(const std::string& filename);
    void processMIDIMessage(DWORD param);
    void setVolume(double volume);
    StereoValue render() const;

private:
    std::vector<std::unique_ptr<Channel>> channels_;
    std::vector<std::shared_ptr<const SoundFont>> soundFonts_;
    double volume_;
    std::shared_ptr<const Preset> defaultPreset_, defaultDrumPreset_;
    MIDIStandard midiStandard_;

    std::shared_ptr<const Preset> findPreset(std::uint16_t bank, std::uint8_t presetNum) const;
};

}
