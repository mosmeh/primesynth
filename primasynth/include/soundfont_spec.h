#pragma once
#include <cstdint>

namespace primasynth {
namespace sf {

enum class SampleLink : std::uint16_t {
    monoSample = 1,
    rightSample = 2,
    leftSample = 4,
    linkedSample = 8,
    RomMonoSample = 0x8001,
    RomRightSample = 0x8002,
    RomLeftSample = 0x8004,
    RomLinkedSample = 0x8008
};

enum class Generator : std::uint16_t {
    startAddrsOffset = 0,
    endAddrsOffset = 1,
    startloopAddrsOffset = 2,
    endloopAddrsOffset = 3,
    startAddrsCoarseOffset = 4,
    modLfoToPitch = 5,
    vibLfoToPitch = 6,
    modEnvToPitch = 7,
    initialFilterFc = 8,
    initialFilterQ = 9,
    modLfoToFilterFc = 10,
    modEnvToFilterFc = 11,
    endAddrsCoarseOffset = 12,
    modLfoToVolume = 13,
    chorusEffectsSend = 15,
    reverbEffectsSend = 16,
    pan = 17,
    delayModLFO = 21,
    freqModLFO = 22,
    delayVibLFO = 23,
    freqVibLFO = 24,
    delayModEnv = 25,
    attackModEnv = 26,
    holdModEnv = 27,
    decayModEnv = 28,
    sustainModEnv = 29,
    releaseModEnv = 30,
    keynumToModEnvHold = 31,
    keynumToModEnvDecay = 32,
    delayVolEnv = 33,
    attackVolEnv = 34,
    holdVolEnv = 35,
    decayVolEnv = 36,
    sustainVolEnv = 37,
    releaseVolEnv = 38,
    keynumToVolEnvHold = 39,
    keynumToVolEnvDecay = 40,
    instrument = 41,
    keyRange = 43,
    velRange = 44,
    startloopAddrsCoarseOffset = 45,
    keynum = 46,
    velocity = 47,
    initialAttenuation = 48,
    endloopAddrsCoarseOffset = 50,
    coarseTune = 51,
    fineTune = 52,
    sampleID = 53,
    sampleModes = 54,
    scaleTuning = 56,
    exclusiveClass = 57,
    overridingRootKey = 58,
    pitch = 59,
    endOper = 60
};

enum class GeneralController : std::uint8_t {
    noController = 0,
    noteOnVelocity = 2,
    noteOnKeyNumber = 3,
    polyPressure = 10,
    channelPressure = 13,
    pitchWheel = 14,
    pitchWheelSensitivity = 16,
    link = 127
};

enum class ControllerPalette {
    general = 0,
    midi = 1
};

enum class SourceDirection {
    positive = 0,
    negative = 1
};

enum class SourcePolarity {
    unipolar = 0,
    bipolar = 1
};

enum class SourceType {
    linear = 0,
    concave = 1,
    convex = 2,
    switchType = 3
};

struct Modulator {
    union {
        GeneralController general;
        std::uint8_t midi;
    } index;
    ControllerPalette palette;
    SourceDirection direction;
    SourcePolarity polarity;
    SourceType type;
};

enum class Transform : std::uint16_t {
    linear = 0,
    absoluteValue = 2
};

struct RangesType {
    std::int8_t byLo;
    std::int8_t byHi;
};

union GenAmountType {
    RangesType ranges;
    std::int16_t shAmount;
    std::uint16_t wAmount;
};

struct VersionTag {
    std::uint16_t wMajor;
    std::uint16_t wMinor;
};

struct PresetHeader {
    char achPresetName[20];
    std::uint16_t wPreset;
    std::uint16_t wBank;
    std::uint16_t wPresetBagNdx;
    std::uint32_t dwLibrary;
    std::uint32_t dwGenre;
    std::uint32_t dwMorphology;
};

struct Bag {
    std::uint16_t wGenNdx;
    std::uint16_t wModNdx;
};

struct ModList {
    Modulator sfModSrcOper;
    Generator sfModDestOper;
    std::int16_t modAmount;
    Modulator sfModAmtSrcOper;
    Transform sfModTransOper;
};

struct GenList {
    Generator sfGenOper;
    GenAmountType genAmount;
};

struct Inst {
    char achInstName[20];
    std::uint16_t wInstBagNdx;
};

struct Sample {
    char achSampleName[20];
    std::uint32_t dwStart;
    std::uint32_t dwEnd;
    std::uint32_t dwStartloop;
    std::uint32_t dwEndloop;
    std::uint32_t dwSampleRate;
    std::int8_t byOriginalKey;
    std::int8_t chCorrection;
    std::uint16_t wSampleLink;
    SampleLink sfSampleType;
};

}
}
