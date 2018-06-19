#include "soundfont.h"

namespace primasynth {

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

GeneratorSet::GeneratorSet() {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        generators_.at(i) = {false, DEFAULT_GENERATOR_VALUES.at(i)};
    }
}

std::int16_t GeneratorSet::getOrDefault(sf::Generator type) const {
    return generators_.at(static_cast<std::size_t>(type)).amount;
}

void GeneratorSet::set(sf::Generator type, std::int16_t amount) {
    generators_.at(static_cast<std::size_t>(type)) = {true, amount};
}

void GeneratorSet::merge(const GeneratorSet& b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (!generators_.at(i).used && b.generators_.at(i).used) {
            generators_.at(i) = b.generators_.at(i);
        }
    }
}

void GeneratorSet::add(const GeneratorSet& b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (b.generators_.at(i).used) {
            generators_.at(i).amount += b.generators_.at(i).amount;
            generators_.at(i).used = true;
        }
    }
}

const ModulatorParameterSet& ModulatorParameterSet::getDefaultParameters() {
    static ModulatorParameterSet params;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        {
            // MIDI Note-On Velocity to Initial Attenuation
            sf::ModList mp;
            mp.sfModSrcOper.index.general = sf::GeneralController::noteOnVelocity;
            mp.sfModSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::decrease;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::concaveType;
            mp.sfModDestOper = sf::Generator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Note-On Velocity to Filter Cutoff
            sf::ModList mp;
            mp.sfModSrcOper.index.general = sf::GeneralController::noteOnVelocity;
            mp.sfModSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::decrease;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::initialFilterFc;
            mp.modAmount = -2400;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Channel Pressure to Vibrato LFO Pitch Depth
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 13;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::vibLfoToPitch;
            mp.modAmount = 50;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 1 to Vibrato LFO Pitch Depth
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 1;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::vibLfoToPitch;
            mp.modAmount = 50;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 7 to Initial Attenuation Source
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 7;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::decrease;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::concaveType;
            mp.sfModDestOper = sf::Generator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 10 to Pan Position
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 10;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::bipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::pan;
            mp.modAmount = 500;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 11 to Initial Attenuation
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 11;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::decrease;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::concaveType;
            mp.sfModDestOper = sf::Generator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 91 to Reverb Effects Send
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 91;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::reverbEffectsSend;
            mp.modAmount = 200;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 93 to Reverb Effects Send
            sf::ModList mp;
            mp.sfModSrcOper.index.midi = 93;
            mp.sfModSrcOper.palette = sf::ControllerPalette::midiController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::chorusEffectsSend;
            mp.modAmount = 200;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::noController;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
        {
            // MIDI Pitch Wheel to Initial Pitch Controlled by MIDI Pitch Wheel Sensitivity
            sf::ModList mp;
            mp.sfModSrcOper.index.general = sf::GeneralController::pitchWheel;
            mp.sfModSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModSrcOper.polarity = sf::ControllerPolarity::bipolar;
            mp.sfModSrcOper.type = sf::ControllerType::linearType;
            mp.sfModDestOper = sf::Generator::pitch;
            mp.modAmount = 12700;
            mp.sfModAmtSrcOper.index.general = sf::GeneralController::pitchWheelSensitivity;
            mp.sfModAmtSrcOper.palette = sf::ControllerPalette::generalController;
            mp.sfModAmtSrcOper.direction = sf::ControllerDirection::increase;
            mp.sfModAmtSrcOper.polarity = sf::ControllerPolarity::unipolar;
            mp.sfModAmtSrcOper.type = sf::ControllerType::linearType;
            mp.sfModTransOper = sf::Transform::linear;
            params.append(mp);
        }
    }
    return params;
}

const std::vector<sf::ModList>& ModulatorParameterSet::getParameters() const {
    return modparams_;
}

bool operator==(const sf::Modulator& a, const sf::Modulator& b) {
    return a.index.midi == b.index.midi
        && a.palette == b.palette
        && a.direction == b.direction
        && a.polarity == b.polarity
        && a.type == b.type;
}

bool modulatorsAreIdentical(const sf::ModList& a, const sf::ModList& b) {
    return a.sfModSrcOper == b.sfModSrcOper
        && a.sfModDestOper == b.sfModDestOper
        && a.sfModAmtSrcOper == b.sfModAmtSrcOper
        && a.sfModTransOper == b.sfModTransOper;
}

void ModulatorParameterSet::append(const sf::ModList& modparam) {
    for (const auto& mp : modparams_) {
        if (modulatorsAreIdentical(mp, modparam)) {
            return;
        }
    }
    modparams_.push_back(modparam);
}

void ModulatorParameterSet::addOrAppend(const sf::ModList& modparam) {
    for (auto& mp : modparams_) {
        if (modulatorsAreIdentical(mp, modparam)) {
            mp.modAmount += modparam.modAmount;
            return;
        }
    }
    modparams_.push_back(modparam);
}

void ModulatorParameterSet::merge(const ModulatorParameterSet& b) {
    for (const auto& mp : b.modparams_) {
        append(mp);
    }
}

void ModulatorParameterSet::mergeAndAdd(const ModulatorParameterSet& b) {
    for (const auto& mp : b.modparams_) {
        addOrAppend(mp);
    }
}

struct RIFFHeader {
    std::uint32_t id;
    std::uint32_t size;
};

RIFFHeader readHeader(std::ifstream& ifs) {
    RIFFHeader header;
    ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
    return header;
}

std::uint32_t readFourCC(std::ifstream& ifs) {
    std::uint32_t id;
    ifs.read(reinterpret_cast<char*>(&id), sizeof(id));
    return id;
}

constexpr std::uint32_t toFourCC(const char str[5]) {
    std::uint32_t fourCC = 0;
    for (int i = 0; i < 4; ++i) {
        fourCC |= str[i] << CHAR_BIT * i;
    }
    return fourCC;
}

SoundFont::SoundFont(const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) {
        throw std::runtime_error("failed to open file");
    }

    const RIFFHeader riffHeader = readHeader(ifs);
    const std::uint32_t riffType = readFourCC(ifs);
    if (riffHeader.id != toFourCC("RIFF") || riffType != toFourCC("sfbk")) {
        throw std::runtime_error("not a SoundFont file");
    }

    for (std::size_t s = 0; s < riffHeader.size - sizeof(riffType);) {
        const RIFFHeader chunkHeader = readHeader(ifs);
        s += sizeof(chunkHeader) + chunkHeader.size;
        switch (chunkHeader.id) {
        case toFourCC("LIST"): {
            const std::uint32_t chunkType = readFourCC(ifs);
            const std::size_t chunkSize = chunkHeader.size - sizeof(chunkType);
            switch (chunkType) {
            case toFourCC("INFO"):
                readInfoChunk(ifs, chunkSize);
                break;
            case toFourCC("sdta"):
                readSdtaChunk(ifs, chunkSize);
                break;
            case toFourCC("pdta"):
                readPdtaChunk(ifs, chunkSize);
                break;
            default:
                ifs.ignore(chunkSize);
                break;
            }
            break;
        }
        default:
            ifs.ignore(chunkHeader.size);
            break;
        }
    }
}

const std::string& SoundFont::getName() const {
    return name_;
}

const std::vector<Sample>& SoundFont::getSamples() const {
    return samples_;
}

const std::vector<Instrument>& SoundFont::getInstruments() const {
    return instruments_;
}

const std::vector<std::shared_ptr<const Preset>>& SoundFont::getPresetPtrs() const {
    return presets_;
}

void SoundFont::readInfoChunk(std::ifstream& ifs, std::size_t size) {
    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("ifil"): {
            sf::VersionTag ver;
            ifs.read(reinterpret_cast<char*>(&ver), subchunkHeader.size);
            if (ver.wMajor > 2 || ver.wMinor > 4) {
                throw std::runtime_error("SoundFont later than 2.04 not supported");
            }
            break;
        }
        case toFourCC("INAM"): {
            std::vector<char> buf(subchunkHeader.size);
            ifs.read(buf.data(), buf.size());
            name_ = buf.data();
            break;
        }
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }
}

void SoundFont::readSdtaChunk(std::ifstream& ifs, std::size_t size) {
    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("smpl"):
            sampleBuffer_.resize(subchunkHeader.size / sizeof(std::int16_t));
            ifs.read(reinterpret_cast<char*>(sampleBuffer_.data()), subchunkHeader.size);
            break;
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }
}

std::string achToString(const char ach[20]) {
    return {ach, strnlen(ach, 20)};
}

template <typename T>
void readPdtaList(std::ifstream& ifs, std::vector<T>& list, std::uint32_t totalSize, std::size_t structSize) {
    if (totalSize % structSize != 0) {
        throw std::runtime_error("invalid chunk size");
    }
    list.resize(totalSize / structSize);
    for (std::size_t i = 0; i < totalSize / structSize; ++i) {
        ifs.read(reinterpret_cast<char*>(&list.at(i)), structSize);
    }
}

void readModulator(std::ifstream& ifs, sf::Modulator& mod) {
    std::uint16_t data;
    ifs.read(reinterpret_cast<char*>(&data), 2);

    mod.index.midi = data & 127;
    mod.palette = static_cast<sf::ControllerPalette>((data >> 7) & 1);
    mod.direction = static_cast<sf::ControllerDirection>((data >> 8) & 1);
    mod.polarity = static_cast<sf::ControllerPolarity>((data >> 9) & 1);
    mod.type = static_cast<sf::ControllerType>((data >> 10) & 63);
}

void readModList(std::ifstream& ifs, std::vector<sf::ModList>& list, std::uint32_t totalSize) {
    static const size_t STRUCT_SIZE = 10;
    if (totalSize % STRUCT_SIZE != 0) {
        throw std::runtime_error("invalid chunk size");
    }
    list.reserve(totalSize / STRUCT_SIZE);
    for (std::size_t i = 0; i < totalSize / STRUCT_SIZE; ++i) {
        sf::ModList mod;
        readModulator(ifs, mod.sfModSrcOper);
        ifs.read(reinterpret_cast<char*>(&mod.sfModDestOper), 2);
        ifs.read(reinterpret_cast<char*>(&mod.modAmount), 2);
        readModulator(ifs, mod.sfModAmtSrcOper);
        ifs.read(reinterpret_cast<char*>(&mod.sfModTransOper), 2);
        list.push_back(mod);
    }
}

template <typename ItBag, typename ItGen, typename ItMod>
void readBags(std::vector<Zone>& zones, const ItBag& bagBegin, const ItBag& bagEnd,
    const ItGen& genBegin, const ItMod& modBegin, sf::Generator indexGen) {

    Zone globalZone;
    for (auto it_bag = bagBegin; it_bag != bagEnd; ++it_bag) {
        Zone zone;
        const auto& beginGen = genBegin + it_bag->wGenNdx;
        const auto& endGen = genBegin + std::next(it_bag)->wGenNdx;
        for (auto it_gen = beginGen; it_gen != endGen; ++it_gen) {
            switch (it_gen->sfGenOper) {
            case sf::Generator::keyRange: {
                const auto& range = it_gen->genAmount.ranges;
                zone.keyRange = {range.byLo, range.byHi};
                break;
            }
            case sf::Generator::velRange: {
                const auto& range = it_gen->genAmount.ranges;
                zone.velocityRange = {range.byLo, range.byHi};
                break;
            }
            default:
                zone.generators.set(it_gen->sfGenOper, it_gen->genAmount.shAmount);
                break;
            }
        }
        const auto& beginMod = modBegin + it_bag->wModNdx;
        const auto& endMod = modBegin + std::next(it_bag)->wModNdx;
        for (auto it_mod = beginMod; it_mod != endMod; ++it_mod) {
            zone.modulatorParameters.append(*it_mod);
        }
        if (beginGen != endGen && std::prev(endGen)->sfGenOper == indexGen) {
            zones.push_back(zone);
        } else if (it_bag == bagBegin && (beginGen != endGen || beginMod != endMod)) {
            globalZone = zone;
        }
    }
    for (auto& zone : zones) {
        zone.generators.merge(globalZone.generators);
        zone.modulatorParameters.merge(globalZone.modulatorParameters);
    }
}

void SoundFont::readPdtaChunk(std::ifstream& ifs, std::size_t size) {
    std::vector<sf::PresetHeader> phdr;
    std::vector<sf::Inst> inst;
    std::vector<sf::Bag> pbag, ibag;
    std::vector<sf::ModList> pmod, imod;
    std::vector<sf::GenList> pgen, igen;
    std::vector<sf::Sample> shdr;

    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("phdr"):
            readPdtaList(ifs, phdr, subchunkHeader.size, 38);
            break;
        case toFourCC("pbag"):
            readPdtaList(ifs, pbag, subchunkHeader.size, 4);
            break;
        case toFourCC("pmod"):
            readModList(ifs, pmod, subchunkHeader.size);
            break;
        case toFourCC("pgen"):
            readPdtaList(ifs, pgen, subchunkHeader.size, 4);
            break;
        case toFourCC("inst"):
            readPdtaList(ifs, inst, subchunkHeader.size, 22);
            break;
        case toFourCC("ibag"):
            readPdtaList(ifs, ibag, subchunkHeader.size, 4);
            break;
        case toFourCC("imod"):
            readModList(ifs, imod, subchunkHeader.size);
            break;
        case toFourCC("igen"):
            readPdtaList(ifs, igen, subchunkHeader.size, 4);
            break;
        case toFourCC("shdr"):
            readPdtaList(ifs, shdr, subchunkHeader.size, 46);
            break;
        default:
            ifs.ignore(subchunkHeader.size);
            break;
        }
    }

    instruments_.reserve(inst.size() - 1);
    for (auto it_inst = inst.begin(); it_inst != std::prev(inst.end()); ++it_inst) {
        instruments_.emplace_back();
        auto& instrument = instruments_.back();
        instrument.name = achToString(it_inst->achInstName);

        const auto& bagBegin = ibag.begin() + it_inst->wInstBagNdx;
        const auto& bagEnd = ibag.begin() + std::next(it_inst)->wInstBagNdx;
        readBags(instrument.zones, bagBegin, bagEnd, igen.begin(), imod.begin(), sf::Generator::sampleID);
    }

    presets_.reserve(phdr.size() - 1);
    for (auto it_phdr = phdr.begin(); it_phdr != std::prev(phdr.end()); ++it_phdr) {
        const auto preset = std::make_shared<Preset>(*this);
        preset->name = achToString(it_phdr->achPresetName);
        preset->bank = it_phdr->wBank;
        preset->presetNum = it_phdr->wPreset;

        const auto& bagBegin = pbag.begin() + it_phdr->wPresetBagNdx;
        const auto& bagEnd = pbag.begin() + std::next(it_phdr)->wPresetBagNdx;
        readBags(preset->zones, bagBegin, bagEnd, pgen.begin(), pmod.begin(), sf::Generator::instrument);

        presets_.push_back(preset);
    }

    samples_.reserve(shdr.size() - 1);
    for (auto it_shdr = shdr.begin(); it_shdr < std::prev(shdr.end()); ++it_shdr) {
        samples_.emplace_back(sampleBuffer_);
        auto& sample = samples_.back();
        sample.name = achToString(it_shdr->achSampleName);
        sample.start = it_shdr->dwStart;
        sample.end = it_shdr->dwEnd;
        sample.startLoop = it_shdr->dwStartloop;
        sample.endLoop = it_shdr->dwEndloop;
        sample.sampleRate = it_shdr->dwSampleRate;
        sample.key = it_shdr->byOriginalKey;
        sample.correction = it_shdr->chCorrection;

        int sampleMax = 0;
        for (std::size_t i = sample.start; i < sample.end; ++i) {
            sampleMax = std::max(sampleMax, std::abs(sampleBuffer_.at(i)));
        }
        sample.minAtten = -2.0 / 9.6 * std::log10(static_cast<double>(sampleMax) / INT16_MAX);
    }
}

}
