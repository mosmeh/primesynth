#pragma once

namespace primasynth {
namespace midi {

static constexpr std::uint8_t PERCUSSION_CHANNEL = 9;
static constexpr std::size_t NUM_CONTROLLERS = 128;
static constexpr std::uint8_t MAX_KEY = 127;

enum class Standard {
    GM,
    GS,
    XG
};

enum class MessageStatus {
    NoteOff = 0x80,
    NoteOn = 0x90,
    KeyPressure = 0xa0,
    ControlChange = 0xb0,
    ProgramChange = 0xc0,
    ChannelPressure = 0xd0,
    PitchBend = 0xe0
};

// GM CCs + bank select + RPN/NRPN
enum class ControlChange {
    BankSelectMSB = 0,
    Modulation = 1,
    DataEntryMSB = 6,
    Volume = 7,
    Pan = 10,
    Expression = 11,
    BankSelectLSB = 32,
    DataEntryLSB = 38,
    Sustain = 64,
    NRPNLSB = 98,
    NRPNMSB = 99,
    RPNLSB = 100,
    RPNMSB = 101,
    AllSoundOff = 120,
    ResetAllControllers = 121,
    AllNotesOff = 123
};

// GM RPNs
enum class RPN {
    PitchBendSensitivity = 0,
    FineTuning = 1,
    CoarseTuning = 2
};

struct Bank {
    std::uint8_t msb, lsb;
};

}
}
