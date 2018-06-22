#pragma once
#include "ring_buffer.h"
#include "synthesizer.h"
#include "third_party/portaudio.h"
#include <atomic>

namespace primasynth {
class AudioOutput {
public:
    AudioOutput(Synthesizer& synth, std::size_t bufferSize, int deviceID = getDefaultDeviceID(),
                double sampleRate = getDefaultSampleRate());
    ~AudioOutput();

    static int getDefaultDeviceID();
    static double getDefaultSampleRate();

private:
    RingBuffer buffer_;
    PaStream* stream_;
    std::thread renderingThread;
    std::atomic_bool running_;
};
}
