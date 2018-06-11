#pragma once
#include <mutex>
#include "midi.h"
#include "voice.h"

namespace primasynth {

struct Bank {
    std::uint8_t msb, lsb;
};

class Channel {
public:
    Channel(double outputRate, bool percussion);

    bool isPercussionChannel() const;
    Bank getBank() const;

    void noteOn(std::uint8_t key, std::uint8_t velocity);
    void noteOff(std::uint8_t key);
    void controlChange(std::uint8_t controller, std::uint8_t value);
    void pitchBend(std::uint16_t value);
    void channelPressure(std::uint8_t value);
    void setPreset(const std::shared_ptr<const Preset>& preset);
    void update();
    StereoValue render();

private:
    enum class DataEntryMode {
        RPN,
        NRPN
    };

    const double outputRate_;
    const bool percussion_;
    std::shared_ptr<const Preset> preset_;
    std::array<std::uint8_t, NUM_CONTROLLERS> controllers_;
    DataEntryMode dataEntryMode_;
    std::uint16_t pitchBend_;
    std::uint8_t channelPressure_;
    std::int16_t pitchBendSensitivity_;
    double fineTuning_, coarseTuning_;
    std::vector<std::unique_ptr<Voice>> voices_;
    std::size_t currentNoteID_;
    std::mutex voiceMutex_;

    void addVoice(std::unique_ptr<Voice> voice);
};

}
