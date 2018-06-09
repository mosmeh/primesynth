#pragma once
#include "soundfont.h"
#include "stereovalue.h"
#include "midi.h"
#include "voice.h"

namespace primasynth {

class Channel {
public:
    explicit Channel(double outputRate, bool drum);

    void noteOn(std::uint8_t key, std::uint8_t velocity);
    void noteOff(std::uint8_t key);
    void controlChange(std::uint8_t controller, std::uint8_t value);
    void pitchBend(std::uint16_t value);
    void channelPressure(std::uint8_t value);
    void setPreset(const std::shared_ptr<const Preset>& preset);
    void update();
    StereoValue render();
    bool isDrumChannel() const;
    std::uint16_t getBank() const;

private:
    const double outputRate_;
    const bool drum_;
    std::uint16_t bank_;
    std::shared_ptr<const Preset> preset_;
    std::array<std::uint8_t, NUM_CONTROLLERS> controllers_;
    std::uint16_t pitchBend_;
    std::uint8_t channelPressure_;
    std::int16_t pitchBendSensitivity_;
    std::vector<std::unique_ptr<Voice>> voices_;
    std::size_t currentNoteID_;
    std::mutex voiceMutex_;

    void addVoice(std::unique_ptr<Voice> voice, std::int16_t exclusiveClass);
};

}
