#pragma once
#include <atomic>
#define NOMINMAX
#include <Windows.h>
#include "synthesizer.h"

namespace primasynth {

class MIDIInput {
public:
    struct CallbackParam {
        Synthesizer& synth;
        std::atomic_bool running;
    };

    MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose = false);
    ~MIDIInput();

private:
    HMIDIIN hmi_;
    std::vector<char> sysExBuffer_;
    MIDIHDR mh_;
    CallbackParam callbackParam_;
};

}
