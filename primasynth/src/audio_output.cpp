#include "audio_output.h"
#include <iostream>
#include <sstream>
#define NOMINMAX
#include <Windows.h>

namespace primasynth {
int streamCallback(const void*, void* output, unsigned long frameCount, const PaStreamCallbackTimeInfo*,
                   PaStreamCallbackFlags, void* userData) {
    const auto out = static_cast<float*>(output);
    const auto buffer = reinterpret_cast<RingBuffer*>(userData);
    for (unsigned long i = 0; i < 2 * frameCount; ++i) {
        out[i] = buffer->empty() ? 0 : buffer->shift();
    }
    return PaStreamCallbackResult::paContinue;
}

void doRenderingLoop(std::atomic_bool& running, const Synthesizer& synth, RingBuffer& buffer, double sampleRate) {
    static const int UNIT_STEPS = 64;
    const double stepDuration = UNIT_STEPS / sampleRate;

    double aheadDuration = 0.0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (running) {
        for (int i = 0; i < UNIT_STEPS && !buffer.full(); ++i) {
            const StereoValue sample = synth.render();
            buffer.push(static_cast<float>(sample.left));
            buffer.push(static_cast<float>(sample.right));
        }

        auto now = std::chrono::high_resolution_clock::now();
        aheadDuration += stepDuration - 2.0 * std::chrono::duration<double>(now - lastTime).count();
        lastTime = now;
        aheadDuration = std::max(aheadDuration, 0.0);

        if (aheadDuration > 1.0) {
            std::this_thread::sleep_for(std::chrono::duration<double>(stepDuration));
        }
    }
}

void checkPaError(PaError error) {
    if (error != paNoError) {
        std::ostringstream ss;
        ss << "PortAudio: " << Pa_GetErrorText(error);
        throw std::runtime_error(ss.str());
    }
}

AudioOutput::AudioOutput(Synthesizer& synth, std::size_t bufferSize, int deviceID, double sampleRate)
    : buffer_(bufferSize), running_(true) {
    PaStreamParameters params = {};
    params.channelCount = 2;
    params.sampleFormat = paFloat32;

    if (deviceID > 0) {
        if (deviceID >= Pa_GetDeviceCount()) {
            throw std::runtime_error("invalid output device ID");
        }
        params.device = deviceID;
    } else {
        params.device = Pa_GetDefaultOutputDevice();
    }

    const auto deviceInfo = Pa_GetDeviceInfo(params.device);
    params.suggestedLatency = deviceInfo->defaultLowOutputLatency;

    const UINT cp = GetConsoleCP();
    SetConsoleOutputCP(CP_UTF8);
    printf("Audio: opening %s (%s, %.0fHz)\n", deviceInfo->name, Pa_GetHostApiInfo(deviceInfo->hostApi)->name,
           sampleRate);
    SetConsoleOutputCP(cp);
    checkPaError(Pa_OpenStream(&stream_, nullptr, &params, sampleRate, paFramesPerBufferUnspecified, paNoFlag,
                               streamCallback, &buffer_));

    renderingThread = std::thread(doRenderingLoop, std::ref(running_), std::ref(synth), std::ref(buffer_), sampleRate);

    checkPaError(Pa_StartStream(stream_));
}

AudioOutput::~AudioOutput() {
    running_ = false;
    checkPaError(Pa_StopStream(stream_));

    if (renderingThread.joinable()) {
        renderingThread.join();
    }
}

int AudioOutput::getDefaultDeviceID() {
    return Pa_GetDefaultOutputDevice();
}

double AudioOutput::getDefaultSampleRate() {
    return Pa_GetDeviceInfo(getDefaultDeviceID())->defaultSampleRate;
}

static class PortAudioInstance {
public:
    PortAudioInstance() {
        checkPaError(Pa_Initialize());
    }

    ~PortAudioInstance() {
        checkPaError(Pa_Terminate());
    }
} paInstance;
}
