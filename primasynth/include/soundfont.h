#pragma once
#include "sfspec.h"

namespace primasynth {

class GeneratorSet {
public:
    GeneratorSet();

    std::int16_t getOrDefault(SFGenerator type) const;

    void set(SFGenerator type, std::int16_t amount);
    void merge(const GeneratorSet& b);
    void mergeAndAdd(const GeneratorSet& b);

private:
    struct Generator {
        bool used = false;
        std::int16_t amount = 0;
    };
    std::array<Generator, NUM_GENERATORS> generators_;
};

class ModulatorParameterSet {
public:
    static const ModulatorParameterSet& getDefaultParameters();

    const std::vector<sfModList>& getParameters() const;

    void append(const sfModList& modparam);
    void addOrAppend(const sfModList& modparam);
    void merge(const ModulatorParameterSet& b);
    void mergeAndAdd(const ModulatorParameterSet& b);

private:
    std::vector<sfModList> modparams_;
};

struct Zone {
    struct Range {
        std::int8_t min = 0, max = 127;

        bool contains(std::int8_t value) const {
            return min <= value && value <= max;
        }
    };

    Range keyRange, velocityRange;
    GeneratorSet generators;
    ModulatorParameterSet modulatorParameters;

    bool isInRange(std::int8_t key, std::int8_t velocity) const {
        return keyRange.contains(key) && velocityRange.contains(velocity);
    }
};

struct Instrument {
    std::string name;
    std::vector<Zone> zones;
};

class SoundFont;

struct Preset {
    const SoundFont* soundFont;
    std::string name;
    std::uint16_t bank, presetNum;
    std::vector<Zone> zones;
};

struct Sample {
    const SoundFont* soundFont;
    std::string name;
    std::uint32_t start, end, startLoop, endLoop, sampleRate;
    std::int8_t key, correction;
};

enum class SampleMode {
    UnLooped,
    Looped,
    UnUsed,
    LoopedWithRemainder
};

class SoundFont {
public:
    explicit SoundFont(const std::string& filename);

    const std::string& getName() const;
    const std::vector<std::int16_t>& getSampleBuffer() const;
    const std::vector<std::shared_ptr<const Sample>>& getSamples() const;
    const std::vector<std::shared_ptr<const Instrument>>& getInstruments() const;
    const std::vector<std::shared_ptr<const Preset>>& getPresets() const;

private:
    std::string name_;
    std::vector<std::int16_t> sampleBuffer_;
    std::vector<std::shared_ptr<const Sample>> samples_;
    std::vector<std::shared_ptr<const Instrument>> instruments_;
    std::vector<std::shared_ptr<const Preset>> presets_;

    void readInfoChunk(std::ifstream& ifs, std::size_t size);
    void readSdtaChunk(std::ifstream& ifs, std::size_t size);
    void readPdtaChunk(std::ifstream& ifs, std::size_t size);
};

}
