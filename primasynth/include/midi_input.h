#pragma once
#define NOMINMAX
#include <Windows.h>
#include "synthesizer.h"

namespace primasynth {

class MIDIInput {
public:
    MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose = false);
    ~MIDIInput();

private:
    HMIDIIN hmi_;
};

}
