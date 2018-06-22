#pragma once
#include "synthesizer.h"
#include <atomic>
#define NOMINMAX
#include <Windows.h>

namespace primasynth {
class MIDIInput {
public:
    struct SharedParam {
        Synthesizer& synth;
        std::atomic_bool running;
        bool addingBufferRequested;
        std::mutex mutex;
        std::condition_variable cv;
    };

    MIDIInput(Synthesizer& synth, UINT deviceID, bool verbose = false);
    ~MIDIInput();

private:
    HMIDIIN hmi_;
    std::vector<char> sysExBuffer_;
    MIDIHDR mh_;
    SharedParam sharedParam_;
    std::thread bufferAddingThread_;
};
}
