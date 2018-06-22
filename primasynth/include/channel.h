#pragma once
#include "midi.h"
#include "voice.h"
#include <mutex>
#include <vector>

namespace primasynth {
class Channel {
public:
    explicit Channel(double outputRate);

    midi::Bank getBank() const;
    bool hasPreset() const;

    void noteOff(std::uint8_t key);
    void noteOn(std::uint8_t key, std::uint8_t velocity);
    void keyPressure(std::uint8_t key, std::uint8_t value);
    void controlChange(std::uint8_t controller, std::uint8_t value);
    void channelPressure(std::uint8_t value);
    void pitchBend(std::uint16_t value);
    void setPreset(const std::shared_ptr<const Preset>& preset);
    void update();
    StereoValue render();

private:
    enum class DataEntryMode { RPN, NRPN };

    const double outputRate_;
    std::shared_ptr<const Preset> preset_;
    std::array<std::uint8_t, midi::NUM_CONTROLLERS> controllers_;
    std::array<std::uint8_t, midi::MAX_KEY + 1> keyPressures_;
    std::uint8_t channelPressure_;
    std::uint16_t pitchBend_;
    DataEntryMode dataEntryMode_;
    std::int16_t pitchBendSensitivity_;
    double fineTuning_, coarseTuning_;
    std::vector<std::unique_ptr<Voice>> voices_;
    std::size_t currentNoteID_;
    std::mutex voiceMutex_;

    void addVoice(std::unique_ptr<Voice> voice);
};
}
