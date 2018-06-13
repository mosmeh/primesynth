#include "midi_input.h"
#include <iostream>
#include <sstream>

namespace primasynth {

void checkMMResult(MMRESULT result) {
    if (result != MMSYSERR_NOERROR) {
        char msg[MAXERRORLENGTH];
        midiInGetErrorTextA(result, msg, sizeof(msg));
        std::ostringstream ss;
        ss << "MIDI: " << msg;
        throw std::runtime_error(ss.str());
    }
}

void CALLBACK MidiInProc(HMIDIIN, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD) {
    switch (wMsg) {
    case MIM_DATA:
        reinterpret_cast<Synthesizer*>(dwInstance)->processMIDIMessage(dwParam1);
        break;
    }
}

void CALLBACK verboseMidiInProc(HMIDIIN, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD) {
    switch (wMsg) {
    case MIM_DATA: {
        const auto msg = reinterpret_cast<std::uint8_t*>(&dwParam1);
        const auto status = static_cast<midi::MessageStatus>(msg[0] & 0xf0);
        const auto channel = msg[0] & 0xf;
        switch (status) {
        case midi::MessageStatus::NoteOff:
            std::cout << "Note off: channel=" << channel
                << " key=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::NoteOn:
            std::cout << "Note on: channel=" << channel
                << " key=" << static_cast<int>(msg[1])
                << " velocity=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case midi::MessageStatus::ControlChange:
            std::cout << "Control change: channel=" << channel
                << " controller=" << static_cast<int>(msg[1])
                << " value=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case midi::MessageStatus::ProgramChange:
            std::cout << "Program change: channel=" << channel
                << " program=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::ChannelPressure:
            std::cout << "Channel pressure: channel=" << channel
                << " value=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::PitchBend:
            std::cout << "Pitch bend: channel=" << channel
                << " value=" << conv::joinBytes(msg[2], msg[1]) << std::endl;
            break;
        }
        reinterpret_cast<Synthesizer*>(dwInstance)->processMIDIMessage(dwParam1);
        break;
    }
    }
}

MIDIInput::MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose) {
    MIDIINCAPS caps;
    checkMMResult(midiInGetDevCaps(deviceID, &caps, sizeof(caps)));
    std::wcout << "MIDI: opening " << caps.szPname << std::endl;
    checkMMResult(midiInOpen(&hmi_, deviceID,
        reinterpret_cast<DWORD_PTR>(verbose ? verboseMidiInProc : MidiInProc),
        reinterpret_cast<DWORD_PTR>(&synth), CALLBACK_FUNCTION));
    checkMMResult(midiInStart(hmi_));
}

MIDIInput::~MIDIInput() {
    if (hmi_ != NULL) {
        midiInStop(hmi_);
        midiInReset(hmi_);
        midiInClose(hmi_);
    }
}

}
