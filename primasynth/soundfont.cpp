#include "soundfont.h"

namespace primasynth {

GeneratorSet::GeneratorSet() {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        generators_.at(i).amount = DEFAULT_GENERATOR_VALUES.at(i);
    }
}

void GeneratorSet::set(SFGenerator type, std::int16_t amount) {
    generators_.at(static_cast<std::size_t>(type)) = {true, amount};
}

void GeneratorSet::merge(const GeneratorSet& b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (!generators_.at(i).used && b.generators_.at(i).used) {
            generators_.at(i) = b.generators_.at(i);
        }
    }
}

void GeneratorSet::mergeAndAdd(const GeneratorSet& b) {
    for (std::size_t i = 0; i < NUM_GENERATORS; ++i) {
        if (b.generators_.at(i).used) {
            if (generators_.at(i).used) {
                generators_.at(i).amount += b.generators_.at(i).amount;
            } else {
                generators_.at(i) = b.generators_.at(i);
            }
        }
    }
}

std::int16_t GeneratorSet::getOrDefault(SFGenerator type) const {
    return generators_.at(static_cast<std::size_t>(type)).amount;
}

bool operator==(const SFModulator& a, const SFModulator& b) {
    return a.index.midi == b.index.midi
        && a.palette == b.palette
        && a.direction == b.direction
        && a.polarity == b.polarity
        && a.type == b.type;
}

bool modulatorsAreIdentical(const sfModList& a, const sfModList& b) {
    return a.sfModSrcOper == b.sfModSrcOper
        && a.sfModDestOper == b.sfModDestOper
        && a.sfModAmtSrcOper == b.sfModAmtSrcOper
        && a.sfModTransOper == b.sfModTransOper;
}

void ModulatorParameterSet::append(const sfModList& modparam) {
    for (const auto& mp : modparams_) {
        if (modulatorsAreIdentical(mp, modparam)) {
            return;
        }
    }
    modparams_.push_back(modparam);
}

void ModulatorParameterSet::addOrAppend(const sfModList& modparam) {
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

const std::vector<sfModList>& ModulatorParameterSet::getParameters() const {
    return modparams_;
}

const ModulatorParameterSet& ModulatorParameterSet::getDefaultParameters() {
    static ModulatorParameterSet params;
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
        {
            // MIDI Note-On Velocity to Initial Attenuation
            sfModList mp;
            mp.sfModSrcOper.index.general = SFGeneralController::noteOnVelocity;
            mp.sfModSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModSrcOper.direction = SFControllerDirection::decrease;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::concaveType;
            mp.sfModDestOper = SFGenerator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Note-On Velocity to Filter Cutoff
            sfModList mp;
            mp.sfModSrcOper.index.general = SFGeneralController::noteOnVelocity;
            mp.sfModSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModSrcOper.direction = SFControllerDirection::decrease;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::initialFilterFc;
            mp.modAmount = -2400;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Channel Pressure to Vibrato LFO Pitch Depth
            sfModList mp;
            mp.sfModSrcOper.index.midi = 13;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::vibLfoToPitch;
            mp.modAmount = 50;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 1 to Vibrato LFO Pitch Depth
            sfModList mp;
            mp.sfModSrcOper.index.midi = 1;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::vibLfoToPitch;
            mp.modAmount = 50;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 7 to Initial Attenuation Source
            sfModList mp;
            mp.sfModSrcOper.index.midi = 7;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::decrease;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::concaveType;
            mp.sfModDestOper = SFGenerator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 10 to Pan Position
            sfModList mp;
            mp.sfModSrcOper.index.midi = 10;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::bipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::pan;
            mp.modAmount = 500;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 11 to Initial Attenuation
            sfModList mp;
            mp.sfModSrcOper.index.midi = 11;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::decrease;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::concaveType;
            mp.sfModDestOper = SFGenerator::initialAttenuation;
            mp.modAmount = 960;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 91 to Reverb Effects Send
            sfModList mp;
            mp.sfModSrcOper.index.midi = 91;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::reverbEffectsSend;
            mp.modAmount = 200;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Continuous Controller 93 to Reverb Effects Send
            sfModList mp;
            mp.sfModSrcOper.index.midi = 93;
            mp.sfModSrcOper.palette = SFControllerPalette::midiController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::chorusEffectsSend;
            mp.modAmount = 200;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::noController;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
        {
            // MIDI Pitch Wheel to Initial Pitch Controlled by MIDI Pitch Wheel Sensitivity
            sfModList mp;
            mp.sfModSrcOper.index.general = SFGeneralController::pitchWheel;
            mp.sfModSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModSrcOper.direction = SFControllerDirection::increase;
            mp.sfModSrcOper.polarity = SFControllerPolarity::bipolar;
            mp.sfModSrcOper.type = SFControllerType::linearType;
            mp.sfModDestOper = SFGenerator::pitch;
            mp.modAmount = 12700;
            mp.sfModAmtSrcOper.index.general = SFGeneralController::pitchWheelSensitivity;
            mp.sfModAmtSrcOper.palette = SFControllerPalette::generalController;
            mp.sfModAmtSrcOper.direction = SFControllerDirection::increase;
            mp.sfModAmtSrcOper.polarity = SFControllerPolarity::unipolar;
            mp.sfModAmtSrcOper.type = SFControllerType::linearType;
            mp.sfModTransOper = SFTransform::linear;
            params.append(mp);
        }
    }
    return params;
}

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

    const RIFFHeader riffHeader = readHeader(ifs);
    if (riffHeader.id != toFourCC("RIFF")) {
        throw;
    }
    const std::uint32_t riffType = readFourCC(ifs);
    if (riffType != toFourCC("sfbk")) {
        throw;
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

const std::shared_ptr<std::vector<std::int16_t>>& SoundFont::getSampleBuffer() const {
    return sampleBuffer_;
}

const std::vector<std::shared_ptr<const Sample>>& SoundFont::getSamples() const {
    return samples_;
}

const std::vector<std::shared_ptr<const Instrument>>& SoundFont::getInstruments() const {
    return instruments_;
}

const std::vector<std::shared_ptr<const Preset>>& SoundFont::getPresets() const {
    return presets_;
}

void SoundFont::readInfoChunk(std::ifstream& ifs, std::size_t size) {
    for (std::size_t s = 0; s < size;) {
        const RIFFHeader subchunkHeader = readHeader(ifs);
        s += sizeof(subchunkHeader) + subchunkHeader.size;
        switch (subchunkHeader.id) {
        case toFourCC("ifil"): {
            sfVersionTag ver;
            ifs.read(reinterpret_cast<char*>(&ver), subchunkHeader.size);
            if (ver.wMajor > 2 || ver.wMinor > 4) {
                throw;
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
            sampleBuffer_ = std::make_shared<std::vector<std::int16_t>>(subchunkHeader.size / sizeof(std::int16_t));
            ifs.read(reinterpret_cast<char*>(sampleBuffer_->data()), subchunkHeader.size);
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
        throw;
    }
    list.resize(totalSize / structSize);
    for (std::size_t i = 0; i < totalSize / structSize; ++i) {
        ifs.read(reinterpret_cast<char*>(&list.at(i)), structSize);
    }
}

void readModulator(std::ifstream& ifs, SFModulator& mod) {
    std::uint16_t data;
    ifs.read(reinterpret_cast<char*>(&data), 2);

    mod.index.midi = data & 127;
    mod.palette = static_cast<SFControllerPalette>((data >> 7) & 1);
    mod.direction = static_cast<SFControllerDirection>((data >> 8) & 1);
    mod.polarity = static_cast<SFControllerPolarity>((data >> 9) & 1);
    mod.type = static_cast<SFControllerType>((data >> 10) & 63);
}

void readModList(std::ifstream& ifs, std::vector<sfModList>& list, std::uint32_t totalSize) {
    static const size_t STRUCT_SIZE = 10;
    if (totalSize % STRUCT_SIZE != 0) {
        throw;
    }
    list.reserve(totalSize / STRUCT_SIZE);
    for (std::size_t i = 0; i < totalSize / STRUCT_SIZE; ++i) {
        sfModList mod;
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
    const ItGen& genBegin, const ItMod& modBegin, SFGenerator indexGen) {

    Zone globalZone;
    for (auto it_bag = bagBegin; it_bag != bagEnd; ++it_bag) {
        Zone zone;
        const auto& beginGen = genBegin + it_bag->wGenNdx;
        const auto& endGen = genBegin + std::next(it_bag)->wGenNdx;
        for (auto it_gen = beginGen; it_gen != endGen; ++it_gen) {
            switch (it_gen->sfGenOper) {
            case SFGenerator::keyRange: {
                const auto& range = it_gen->genAmount.ranges;
                zone.keyRange = {range.byLo, range.byHi};
                break;
            }
            case SFGenerator::velRange: {
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
    std::vector<sfPresetHeader> phdr;
    std::vector<sfInst> inst;
    std::vector<sfBag> pbag, ibag;
    std::vector<sfModList> pmod, imod;
    std::vector<sfGenList> pgen, igen;
    std::vector<sfSample> shdr;

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

    for (auto it_inst = inst.begin(); it_inst != std::prev(inst.end()); ++it_inst) {
        const auto instrument = std::make_shared<Instrument>();
        instrument->name = achToString(it_inst->achInstName);

        const auto& bagBegin = ibag.begin() + it_inst->wInstBagNdx;
        const auto& bagEnd = ibag.begin() + std::next(it_inst)->wInstBagNdx;
        readBags(instrument->zones, bagBegin, bagEnd, igen.begin(), imod.begin(), SFGenerator::sampleID);

        instruments_.push_back(instrument);
    }

    for (auto it_phdr = phdr.begin(); it_phdr != std::prev(phdr.end()); ++it_phdr) {
        const auto preset = std::make_shared<Preset>();
        preset->soundFont = this;
        preset->name = achToString(it_phdr->achPresetName);
        preset->bank = it_phdr->wBank;
        preset->presetNum = it_phdr->wPreset;

        const auto& bagBegin = pbag.begin() + it_phdr->wPresetBagNdx;
        const auto& bagEnd = pbag.begin() + std::next(it_phdr)->wPresetBagNdx;
        readBags(preset->zones, bagBegin, bagEnd, pgen.begin(), pmod.begin(), SFGenerator::instrument);

        presets_.push_back(preset);
    }

    for (auto it_shdr = shdr.begin(); it_shdr < std::prev(shdr.end()); ++it_shdr) {
        samples_.emplace_back(std::make_shared<Sample>(Sample{
            this,
            achToString(it_shdr->achSampleName),
            it_shdr->dwStart,
            it_shdr->dwEnd,
            it_shdr->dwStartloop,
            it_shdr->dwEndloop,
            it_shdr->dwSampleRate,
            it_shdr->byOriginalKey,
            it_shdr->chCorrection
        }));
    }
}

}
