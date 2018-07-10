# primesynth
SoundFont-powered MIDI synthesizer

## Usage
```
$ primesynth --help
usage: primesynth [options] ... [soundfonts] ...
options:
  -i, --in            input MIDI device ID (unsigned int [=0])
  -o, --out           output audio device ID (unsigned int [=0])
  -v, --volume        volume (1 = 100%) (double [=1])
  -s, --samplerate    sample rate (Hz) (double [=0])
  -b, --buffer        audio output buffer size (unsigned int [=4096])
  -c, --channels      number of MIDI channels (unsigned int [=16])
      --std           MIDI standard, affects bank selection (gm, gs, xg) (string [=gs])
      --fix-std       do not respond to GM/XG System On, GS Reset, etc.
  -p, --print-msg     print received MIDI messages
  -?, --help          print this message
```

## Installation
Currently primesynth is only for Windows.

Visual Studio supporting C++14 or later is required.
