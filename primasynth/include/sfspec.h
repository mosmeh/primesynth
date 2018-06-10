#pragma once

namespace primasynth {

struct RIFFHeader {
    std::uint32_t id;
    std::uint32_t size;
};

enum class SFSampleLink : std::uint16_t {
    monoSample = 1,
    rightSample = 2,
    leftSample = 4,
    linkedSample = 8,
    RomMonoSample = 0x8001,
    RomRightSample = 0x8002,
    RomLeftSample = 0x8004,
    RomLinkedSample = 0x8008
};

enum class SFGenerator : std::uint16_t {
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

static constexpr std::size_t NUM_GENERATORS = static_cast<std::size_t>(SFGenerator::endOper);

static const std::array<std::int16_t, NUM_GENERATORS> DEFAULT_GENERATOR_VALUES = {
    0,      // startAddrsOffset
    0,      // endAddrsOffset
    0,      // startloopAddrsOffset
    0,      // endloopAddrsOffset
    0,      // startAddrsCoarseOffset
    0,      // modLfoToPitch
    0,      // vibLfoToPitch
    0,      // modEnvToPitch
    13500,  // initialFilterFc
    0,      // initialFilterQ
    0,      // modLfoToFilterFc
    0,      // modEnvToFilterFc
    0,      // endAddrsCoarseOffset
    0,      // modLfoToVolume
    0,      // unused
    0,      // chorusEffectsSend
    0,      // reverbEffectsSend
    0,      // pan
    0,      // unused
    0,      // unused
    0,      // unused
    -12000, // delayModLFO
    0,      // freqModLFO
    -12000, // delayVibLFO
    0,      // freqVibLFO
    -12000, // delayModEnv
    -12000, // attackModEnv
    -12000, // holdModEnv
    -12000, // decayModEnv
    0,      // sustainModEnv
    -12000, // releaseModEnv
    0,      // keynumToModEnvHold
    0,      // keynumToModEnvDecay
    -12000, // delayVolEnv
    -12000, // attackVolEnv
    -12000, // holdVolEnv
    -12000, // decayVolEnv
    0,      // sustainVolEnv
    -12000, // releaseVolEnv
    0,      // keynumToVolEnvHold
    0,      // keynumToVolEnvDecay
    0,      // instrument 
    0,      // reserved
    0,      // keyRange, N/A
    0,      // velRange, N/A
    0,      // startloopAddrsCoarseOffset
    -1,     // keynum
    -1,     // velocity
    0,      // initialAttenuation
    0,      // reserved
    0,      // endloopAddrsCoarseOffset
    0,      // coarseTune
    0,      // fineTune
    0,      // sampleID
    0,      // sampleModes
    0,      // reserved
    100,    // scaleTuning
    0,      // exclusiveClass
    -1,     // overridingRootKey
    0       // pitch
};

enum class SFGeneralController {
    noController = 0,
    noteOnVelocity = 2,
    noteOnKeyNumber = 3,
    polyPressure = 10,
    channelPressure = 13,
    pitchWheel = 14,
    pitchWheelSensitivity = 16,
    link = 127
};

enum class SFControllerPalette {
    generalController = 0,
    midiController = 1
};

enum class SFControllerDirection {
    increase = 0,
    decrease = 1
};

enum class SFControllerPolarity {
    unipolar = 0,
    bipolar = 1
};

enum class SFControllerType {
    linearType = 0,
    concaveType = 1,
    convexType = 2,
    switchType = 3
};

struct SFModulator {
    union {
        SFGeneralController general;
        std::uint8_t midi;
    } index;
    SFControllerPalette palette;
    SFControllerDirection direction;
    SFControllerPolarity polarity;
    SFControllerType type;
};

enum class SFTransform : std::uint16_t {
    linear = 0,
    absoluteValue = 2
};

struct rangesType {
    std::int8_t byLo;
    std::int8_t byHi;
};

union genAmountType {
    rangesType ranges;
    std::int16_t shAmount;
    std::uint16_t wAmount;
};

struct sfVersionTag {
    std::uint16_t wMajor;
    std::uint16_t wMinor;
};

struct sfPresetHeader {
    char achPresetName[20];
    std::uint16_t wPreset;
    std::uint16_t wBank;
    std::uint16_t wPresetBagNdx;
    std::uint32_t dwLibrary;
    std::uint32_t dwGenre;
    std::uint32_t dwMorphology;
};

static constexpr std::uint16_t DRUM_BANK = 128;

struct sfBag {
    std::uint16_t wGenNdx;
    std::uint16_t wModNdx;
};

struct sfModList {
    SFModulator sfModSrcOper;
    SFGenerator sfModDestOper;
    std::int16_t modAmount;
    SFModulator sfModAmtSrcOper;
    SFTransform sfModTransOper;
};

struct sfGenList {
    SFGenerator sfGenOper;
    genAmountType genAmount;
};

struct sfInst {
    char achInstName[20];
    std::uint16_t wInstBagNdx;
};

struct sfSample {
    char achSampleName[20];
    std::uint32_t dwStart;
    std::uint32_t dwEnd;
    std::uint32_t dwStartloop;
    std::uint32_t dwEndloop;
    std::uint32_t dwSampleRate;
    std::int8_t byOriginalKey;
    std::int8_t chCorrection;
    std::uint16_t wSampleLink;
    SFSampleLink sfSampleType;
};

}
