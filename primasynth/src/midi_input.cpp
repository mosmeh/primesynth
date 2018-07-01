#include "midi_input.h"
#include <iomanip>
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
    const auto sp = reinterpret_cast<MIDIInput::SharedParam*>(dwInstance);
    if (!sp->running) {
        return;
    }

    switch (wMsg) {
    case MIM_DATA:
        sp->synth.processShortMessage(dwParam1);
        break;
    case MIM_LONGDATA: {
        const auto mh = reinterpret_cast<LPMIDIHDR>(dwParam1);
        sp->synth.processSysEx(mh->lpData, mh->dwBytesRecorded);

        // See "MidiInProc callback function" (https://msdn.microsoft.com/en-us/library/dd798460.aspx)
        // "Applications should not call any multimedia functions from inside the callback function, as doing so can
        // cause a deadlock"
        std::unique_lock<std::mutex> uniqueLock(sp->mutex);
        sp->addingBufferRequested = true;
        sp->cv.notify_one();

        break;
    }
    }
}

void CALLBACK verboseMidiInProc(HMIDIIN hmi, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2) {
    const auto sp = reinterpret_cast<MIDIInput::SharedParam*>(dwInstance);
    if (!sp->running) {
        return;
    }

    switch (wMsg) {
    case MIM_DATA: {
        const auto msg = reinterpret_cast<std::uint8_t*>(&dwParam1);
        const auto status = static_cast<midi::MessageStatus>(msg[0] & 0xf0);
        const auto channel = msg[0] & 0xf;
        switch (status) {
        case midi::MessageStatus::NoteOff:
            std::cout << "Note off: channel=" << channel << " key=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::NoteOn:
            std::cout << "Note on: channel=" << channel << " key=" << static_cast<int>(msg[1])
                      << " velocity=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case midi::MessageStatus::KeyPressure:
            std::cout << "Key pressure: channel=" << channel << " key=" << static_cast<int>(msg[1])
                      << " value=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case midi::MessageStatus::ControlChange:
            std::cout << "Control change: channel=" << channel << " controller=" << static_cast<int>(msg[1])
                      << " value=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case midi::MessageStatus::ProgramChange:
            std::cout << "Program change: channel=" << channel << " program=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::ChannelPressure:
            std::cout << "Channel pressure: channel=" << channel << " value=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case midi::MessageStatus::PitchBend:
            std::cout << "Pitch bend: channel=" << channel << " value=" << midi::joinBytes(msg[2], msg[1]) << std::endl;
            break;
        }
        break;
    }
    case MIM_LONGDATA: {
        const auto mh = reinterpret_cast<LPMIDIHDR>(dwParam1);
        std::cout << "SysEx: ";
        const auto flags(std::cout.flags());
        for (DWORD i = 0; i < mh->dwBytesRecorded; ++i) {
            std::cout << std::hex << std::setfill('0') << std::setw(2)
                      << static_cast<int>(static_cast<unsigned char>(mh->lpData[i])) << " ";
        }
        std::cout.flags(flags);
        std::cout << std::endl;
        break;
    }
    }

    MidiInProc(hmi, wMsg, dwInstance, dwParam1, dwParam2);
}

MIDIInput::MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose)
    : sysExBuffer_(512), mh_(), sharedParam_{synth, true, false} {
    MIDIINCAPS caps;
    checkMMResult(midiInGetDevCaps(deviceID, &caps, sizeof(caps)));
    std::wcout << "MIDI: opening " << caps.szPname << std::endl;
    checkMMResult(midiInOpen(&hmi_, deviceID, reinterpret_cast<DWORD_PTR>(verbose ? verboseMidiInProc : MidiInProc),
                             reinterpret_cast<DWORD_PTR>(&sharedParam_), CALLBACK_FUNCTION));

    mh_.lpData = sysExBuffer_.data();
    mh_.dwBufferLength = sysExBuffer_.size();

    checkMMResult(midiInPrepareHeader(hmi_, &mh_, sizeof(mh_)));
    checkMMResult(midiInAddBuffer(hmi_, &mh_, sizeof(mh_)));

    bufferAddingThread_ = std::thread([&, &sp = sharedParam_ ] {
        while (true) {
            std::unique_lock<std::mutex> uniqueLock(sp.mutex);
            sp.cv.wait(uniqueLock, [&] { return sp.addingBufferRequested || !sp.running; });
            if (sp.running) {
                checkMMResult(midiInAddBuffer(hmi_, &mh_, sizeof(mh_)));
                sp.addingBufferRequested = false;
            } else {
                break;
            }
        }
    });

    checkMMResult(midiInStart(hmi_));
}

MIDIInput::~MIDIInput() {
    sharedParam_.running = false;
    sharedParam_.cv.notify_all();
    if (bufferAddingThread_.joinable()) {
        bufferAddingThread_.join();
    }

    if (hmi_) {
        checkMMResult(midiInStop(hmi_));
        checkMMResult(midiInReset(hmi_));
        checkMMResult(midiInUnprepareHeader(hmi_, &mh_, sizeof(mh_)));
        checkMMResult(midiInClose(hmi_));
    }
}
}
