#pragma once
#include "sfspec.h"

namespace primasynth {

class SoundFont;

struct Sample {
    const SoundFont* soundFont;
    std::string name;
    std::uint32_t start, end, startLoop, endLoop, sampleRate;
    std::int8_t key, correction;
};

class GeneratorSet {
public:
    void set(SFGenerator type, std::int16_t amount);
    void merge(const GeneratorSet& b);
    void mergeAndAdd(const GeneratorSet& b);
    std::int16_t getOrDefault(SFGenerator type) const;

private:
    struct Generator {
        bool used = false;
        std::int16_t amount = 0;
    };
    std::array<Generator, NUM_GENERATORS> generators_;
};

class ModulatorParameterSet {
public:
    void append(const sfModList& modparam);
    void addOrAppend(const sfModList& modparam);
    void merge(const ModulatorParameterSet& b);
    void mergeAndAdd(const ModulatorParameterSet& b);
    const std::vector<sfModList>& getParameters() const;
    static const ModulatorParameterSet& getDefaultParameters();

private:
    std::vector<sfModList> modparams_;
};

enum class SampleMode {
    UnLooped,
    Looped,
    UnUsed,
    LoopedWithRemainder
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

struct Preset {
    const SoundFont* soundFont;
    std::string name;
    std::uint16_t presetNum, bank;
    std::vector<Zone> zones;
};

class SoundFont {
public:
    explicit SoundFont(const std::string& filename);

    const std::string& getName() const;
    const std::shared_ptr<std::vector<std::int16_t>>& getSampleBuffer() const;
    const std::vector<std::shared_ptr<const Sample>>& getSamples() const;
    const std::vector<std::shared_ptr<const Instrument>>& getInstruments() const;
    const std::vector<std::shared_ptr<const Preset>>& getPresets() const;

private:
    std::string name_;
    std::shared_ptr<std::vector<std::int16_t>> sampleBuffer_;
    std::vector<std::shared_ptr<const Sample>> samples_;
    std::vector<std::shared_ptr<const Instrument>> instruments_;
    std::vector<std::shared_ptr<const Preset>> presets_;

    void readInfoChunk(std::ifstream& ifs, std::size_t size);
    void readSdtaChunk(std::ifstream& ifs, std::size_t size);
    void readPdtaChunk(std::ifstream& ifs, std::size_t size);
};

}
