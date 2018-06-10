# primasynth
SoundFont-powered MIDI synthesizer

## Usage
```
$ primasynth --help
usage: primasynth [options] ... [soundfonts] ...
options:
  -i, --in            input MIDI device ID (unsigned int [=0])
  -o, --out           output audio device ID (unsigned int [=0])
  -v, --volume        volume (1 = 100%) (double [=1])
  -s, --samplerate    sample rate (Hz) (unsigned int [=0])
  -b, --buffer        buffer size (unsigned int [=4096])
  -c, --channels      number of MIDI channels (unsigned int [=16])
      --std           MIDI standard, affects bank selection (gm, gs, xg) (string [=gs])
  -p, --print-midi    print MIDI messages
  -?, --help          print this message
```

## Installation
Currently primasynth is only for Windows.

Visual Studio supporting C++14 or later is required.
