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

void CALLBACK MidiInProc(HMIDIIN hmi, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD) {
    const auto cbParam = reinterpret_cast<MIDIInput::CallbackParam*>(dwInstance);
    if (cbParam->running) {
        switch (wMsg) {
        case MIM_DATA:
            cbParam->synth.processShortMessage(dwParam1);
            break;
        case MIM_LONGDATA: {
            const auto mh = reinterpret_cast<LPMIDIHDR>(dwParam1);
            cbParam->synth.processSysEx(mh->lpData, mh->dwBytesRecorded);
            checkMMResult(midiInAddBuffer(hmi, mh, sizeof(MIDIHDR)));
            break;
        }
        }
    }
}

void CALLBACK verboseMidiInProc(HMIDIIN hmi, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
    if (wMsg == MIM_DATA) {
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
    }

    MidiInProc(hmi, wMsg, dwInstance, dwParam1, dwParam2);
}

MIDIInput::MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose) :
    sysExBuffer_(512),
    mh_(),
    callbackParam_{synth, true} {

    MIDIINCAPS caps;
    checkMMResult(midiInGetDevCaps(deviceID, &caps, sizeof(caps)));
    std::wcout << "MIDI: opening " << caps.szPname << std::endl;
    checkMMResult(midiInOpen(&hmi_, deviceID,
        reinterpret_cast<DWORD_PTR>(verbose ? verboseMidiInProc : MidiInProc),
        reinterpret_cast<DWORD_PTR>(&callbackParam_), CALLBACK_FUNCTION));

    mh_.lpData = sysExBuffer_.data();
    mh_.dwBufferLength = sysExBuffer_.size();

    checkMMResult(midiInPrepareHeader(hmi_, &mh_, sizeof(mh_)));
    checkMMResult(midiInAddBuffer(hmi_, &mh_, sizeof(mh_)));

    checkMMResult(midiInStart(hmi_));
}

MIDIInput::~MIDIInput() {
    callbackParam_.running = false;
    if (hmi_) {
        checkMMResult(midiInStop(hmi_));
        checkMMResult(midiInReset(hmi_));
        checkMMResult(midiInUnprepareHeader(hmi_, &mh_, sizeof(mh_)));
        checkMMResult(midiInClose(hmi_));
    }
}

}
