#pragma once

namespace primasynth {

enum class MIDIMessageStatus {
    NoteOff = 0x80,
    NoteOn = 0x90,
    KeyPressure = 0xa0,
    ControlChange = 0xb0,
    ProgramChange = 0xc0,
    ChannelPressure = 0xd0,
    PitchBend = 0xe0
};

static constexpr size_t NUM_CONTROLLERS = 128;

enum class MIDIControlChange {
    Modulation = 1,
    DataEntryMSB = 6,
    Volume = 7,
    Pan = 10,
    Expression = 11,
    DataEntryLSB = 38,
    Sustain = 64,
    RPNLSB = 100,
    RPNMSB = 101,
    AllSoundOff = 120,
    ResetAllControllers = 121,
    AllNotesOff = 123
};

enum class MIDIRPN {
    PitchBendSensitivity = 0,
    FineTuning = 1,
    CoarseTuning = 2
};

}
