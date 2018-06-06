#include "synth.h"
#include "ringbuffer.h"
#include "cmdline.h"

static_assert(CHAR_BIT == 8, "char is assumed to be 8 bits");

namespace primasynth {

void CALLBACK MidiInProc(HMIDIIN, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD) {
    switch (wMsg) {
    case MIM_OPEN:
        std::cout << "MIDI: opened" << std::endl;
        break;
    case MIM_CLOSE:
        std::cout << "MIDI: closed" << std::endl;
        break;
    case MIM_DATA:
        reinterpret_cast<Synthesizer*>(dwInstance)->processMIDIMessage(dwParam1);
    }
}

int streamCallback(const void*, void* output, unsigned long frameCount,
    const PaStreamCallbackTimeInfo*, PaStreamCallbackFlags, void* userData) {

    const auto out = static_cast<float*>(output);
    const auto buffer = reinterpret_cast<RingBuffer*>(userData);
    for (unsigned long i = 0; i < 2 * frameCount; ++i) {
        out[i] = buffer->empty() ? 0 : buffer->shift();
    }
    return PaStreamCallbackResult::paContinue;
}

}

#define CHECK_PA(x) { \
	const PaError err = x; \
	if (err != paNoError) \
		throw std::runtime_error(Pa_GetErrorText(err)); \
}

int main(int argc, char** argv) {
    using namespace primasynth;

    HMIDIIN hmi = NULL;
    try {
        cmdline::parser argparser;
        argparser.add<double>("volume", 'v', "volume in [0, 1]", false, 1.0);
        argparser.add<int>("buffer", 'b', "buffer size", false, 1 << 12);
        argparser.add<int>("channels", 'c', "number of channels", false, 16);
        argparser.footer("soundfont_filename");
        argparser.parse_check(argc, argv);
        if (argparser.rest().empty()) {
            throw std::runtime_error("SoundFont file required");
        }

        initializeConversionTables();

        CHECK_PA(Pa_Initialize());

        PaStreamParameters params = {};
        params.device = Pa_GetDefaultOutputDevice();
        params.channelCount = 2;
        params.sampleFormat = paFloat32;
        const auto deviceInfo = Pa_GetDeviceInfo(params.device);
        params.suggestedLatency = deviceInfo->defaultLowOutputLatency;
        const double sampleRate = deviceInfo->defaultSampleRate;

        RingBuffer buffer(argparser.get<int>("buffer"));
        Synthesizer synth(sampleRate, argparser.get<int>("channels"));
        synth.loadSoundFont(argparser.rest().at(0));
        synth.setVolume(argparser.get<double>("volume"));

        SetConsoleOutputCP(CP_UTF8);
        printf("opening %s\n", deviceInfo->name);
        std::cout << "API: " << Pa_GetHostApiInfo(deviceInfo->hostApi)->name << std::endl
            << "sample rate: " << deviceInfo->defaultSampleRate << std::endl;

        PaStream* stream;
        CHECK_PA(Pa_OpenStream(&stream, nullptr, &params, sampleRate,
            paFramesPerBufferUnspecified, paNoFlag, streamCallback, &buffer));

        MIDIINCAPS caps;
        const MMRESULT res = midiInGetDevCaps(0, &caps, sizeof(caps));
        if (res != MMSYSERR_NOERROR) {
            TCHAR msg[MAXERRORLENGTH];
            midiInGetErrorText(res, msg, sizeof(msg));
            std::wcerr << "MIDI: " << msg << std::endl;
            throw std::runtime_error("failed to get device capabilities");
        }
        std::wcout << "MIDI: opening " << caps.szPname << std::endl;
        midiInOpen(&hmi, 0,
            reinterpret_cast<DWORD_PTR>(MidiInProc),
            reinterpret_cast<DWORD_PTR>(&synth), CALLBACK_FUNCTION);

        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

        bool running = true;
        std::thread thread([&running, &synth, &buffer, sampleRate] {
            static const int mutexSteps = 64;
            double ahead = 0.0;
            auto lastTime = std::chrono::high_resolution_clock::now();
            while (running) {
                for (int i = 0; i < mutexSteps && !buffer.full(); ++i) {
                    const StereoValue sample = synth.render();
                    buffer.push(static_cast<float>(sample.left));
                    buffer.push(static_cast<float>(sample.right));
                }

                const double stepTime = mutexSteps / sampleRate;
                ahead += stepTime;
                auto currTime = std::chrono::high_resolution_clock::now();
                ahead -= 2.0 * std::chrono::duration<double>(currTime - lastTime).count();
                lastTime = currTime;
                ahead = std::max(ahead, 0.0);

                if (ahead > 1.0) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(stepTime));
                }
            }
        });

        CHECK_PA(Pa_StartStream(stream));
        midiInStart(hmi);
        std::cout << "Press enter to exit" << std::endl;
        std::getchar();
        running = false;
        thread.join();

        CHECK_PA(Pa_StopStream(stream));
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    midiInStop(hmi);
    Pa_Terminate();
    midiInClose(hmi);

    return EXIT_SUCCESS;
}
