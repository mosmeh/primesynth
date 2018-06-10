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
        break;
    }
}

void CALLBACK verboseMidiInProc(HMIDIIN, UINT wMsg, DWORD dwInstance, DWORD dwParam1, DWORD) {
    switch (wMsg) {
    case MIM_OPEN:
        std::cout << "MIDI: opened" << std::endl;
        break;
    case MIM_CLOSE:
        std::cout << "MIDI: closed" << std::endl;
        break;
    case MIM_DATA: {
        const auto msg = reinterpret_cast<std::uint8_t*>(&dwParam1);
        const auto status = static_cast<MIDIMessageStatus>(msg[0] & 0xf0);
        const auto channel = msg[0] & 0xf;
        switch (status) {
        case MIDIMessageStatus::NoteOff:
            std::cout << "Note off: channel=" << channel
                << " key=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case MIDIMessageStatus::NoteOn:
            std::cout << "Note on: channel=" << channel
                << " key=" << static_cast<int>(msg[1])
                << " velocity=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case MIDIMessageStatus::ControlChange:
            std::cout << "Control change: channel=" << channel
                << " controller=" << static_cast<int>(msg[1])
                << " value=" << static_cast<int>(msg[2]) << std::endl;
            break;
        case MIDIMessageStatus::ProgramChange:
            std::cout << "Program change: channel=" << channel
                << " program=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case MIDIMessageStatus::ChannelPressure:
            std::cout << "Channel pressure: channel=" << channel
                << " value=" << static_cast<int>(msg[1]) << std::endl;
            break;
        case MIDIMessageStatus::PitchBend:
            std::cout << "Pitch bend: channel=" << channel
                << " value=" << joinBytes(msg[2], msg[1]) << std::endl;
            break;
        }
        reinterpret_cast<Synthesizer*>(dwInstance)->processMIDIMessage(dwParam1);
        break;
    }
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

void doRenderingLoop(std::atomic_bool& running, const Synthesizer& synth, RingBuffer& buffer, double sampleRate) {
    static const int unitSteps = 64;
    const double stepDuration = unitSteps / sampleRate;

    double aheadDuration = 0.0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    while (running) {
        for (int i = 0; i < unitSteps && !buffer.full(); ++i) {
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

void checkMMError(MMRESULT result) {
    if (result != MMSYSERR_NOERROR) {
        char msg[MAXERRORLENGTH];
        midiInGetErrorTextA(result, msg, sizeof(msg));
        std::ostringstream ss;
        ss << "MIDI: " << msg;
        throw std::runtime_error(ss.str());
    }
}

}

int main(int argc, char** argv) {
    using namespace primasynth;

    HMIDIIN hmi = NULL;
    try {
        checkPaError(Pa_Initialize());

        cmdline::parser argparser;
        argparser.set_program_name("primasynth");
        argparser.add<unsigned int>("in", 'i', "input MIDI device ID", false, 0);
        argparser.add<unsigned int>("out", 'o', "output audio device ID", false);
        argparser.add<double>("volume", 'v', "volume (1 = 100%)", false, 1.0);
        argparser.add<unsigned int>("samplerate", 's', "sample rate (Hz)", false);
        argparser.add<unsigned int>("buffer", 'b', "buffer size", false, 1 << 12);
        argparser.add<unsigned int>("channels", 'c', "number of MIDI channels", false, 16);
        argparser.add<std::string>("midistd", '\0', "MIDI standard (affects bank selection)", false, "gs",
            cmdline::oneof<std::string>("gm", "gs", "xg"));
        argparser.add("print-midi", 'p', "print MIDI messages");
        argparser.footer("[soundfont_filenames] ...");
        argparser.parse_check(argc, argv);
        if (argparser.rest().empty()) {
            throw std::runtime_error("SoundFont file required");
        }

        initializeConversionTables();

        PaStreamParameters params = {};
        params.channelCount = 2;
        params.sampleFormat = paFloat32;

        if (argparser.exist("out")) {
            const auto id = static_cast<int>(argparser.get<unsigned int>("out"));
            if (id >= Pa_GetDeviceCount()) {
                throw std::runtime_error("invalid output device ID");
            }
            params.device = id;
        } else {
            params.device = Pa_GetDefaultOutputDevice();
        }

        const auto deviceInfo = Pa_GetDeviceInfo(params.device);
        params.suggestedLatency = deviceInfo->defaultLowOutputLatency;

        const double sampleRate = argparser.exist("samplerate") ?
            argparser.get<unsigned int>("samplerate") : deviceInfo->defaultSampleRate;

        auto midiStandard = MIDIStandard::GM;
        if (argparser.get<std::string>("midistd") == "gs") {
            midiStandard = MIDIStandard::GS;
        } else if (argparser.get<std::string>("midistd") == "xg") {
            midiStandard = MIDIStandard::XG;
        }

        Synthesizer synth(sampleRate, argparser.get<unsigned int>("channels"), midiStandard);
        for (const std::string& filename : argparser.rest()) {
            synth.loadSoundFont(filename);
        }
        synth.setVolume(argparser.get<double>("volume"));

        RingBuffer buffer(argparser.get<unsigned int>("buffer"));

        const UINT cp = GetConsoleCP();
        SetConsoleOutputCP(CP_UTF8);
        printf("opening %s\n", deviceInfo->name);
        SetConsoleOutputCP(cp);
        std::cout << "API: " << Pa_GetHostApiInfo(deviceInfo->hostApi)->name << std::endl
            << "sample rate: " << sampleRate << "Hz" << std::endl;
        PaStream* stream;
        checkPaError(Pa_OpenStream(&stream, nullptr, &params, sampleRate,
            paFramesPerBufferUnspecified, paNoFlag, streamCallback, &buffer));

        const UINT midiDeviceID = argparser.get<unsigned int>("in");
        MIDIINCAPS caps;
        checkMMError(midiInGetDevCaps(midiDeviceID, &caps, sizeof(caps)));
        std::wcout << "MIDI: opening " << caps.szPname << std::endl;
        checkMMError(midiInOpen(&hmi, midiDeviceID,
            reinterpret_cast<DWORD_PTR>(argparser.exist("print-midi") ? verboseMidiInProc : MidiInProc),
            reinterpret_cast<DWORD_PTR>(&synth), CALLBACK_FUNCTION));

        SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);

        std::atomic_bool running = true;
        std::thread thread(doRenderingLoop, std::ref(running), std::ref(synth), std::ref(buffer), sampleRate);

        checkPaError(Pa_StartStream(stream));
        checkMMError(midiInStart(hmi));
        std::cout << "Press enter to exit" << std::endl;
        std::getchar();

        running = false;
        thread.join();
        checkPaError(Pa_StopStream(stream));
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << std::endl;
    }

    Pa_Terminate();
    if (hmi != NULL) {
        midiInStop(hmi);
        midiInClose(hmi);
    }

    return EXIT_SUCCESS;
}
