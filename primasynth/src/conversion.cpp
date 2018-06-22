#include "conversion.h"
#include <array>

namespace primasynth {
namespace conv {
std::array<double, 1441> attenToAmpTable;
std::array<double, 1200> centToHeltzTable;

void initialize() {
    static bool initialized = false;
    if (!initialized) {
        initialized = true;

        for (std::size_t i = 0; i < attenToAmpTable.size(); ++i) {
            // -200 instead of -100 for compatibility
            attenToAmpTable.at(i) = std::pow(10.0, i / -200.0);
        }
        for (std::size_t i = 0; i < centToHeltzTable.size(); i++) {
            centToHeltzTable.at(i) = 6.875 * std::exp2(i / 1200.0);
        }
    }
}

double attenToAmp(double atten) {
    if (atten <= 0) {
        return 1.0;
    } else if (atten >= attenToAmpTable.size()) {
        return 0.0;
    } else {
        return attenToAmpTable.at(static_cast<std::size_t>(atten));
    }
}

double ampToNormedAtten(double amp) {
    return -20.0 / 96.0 * std::log10(amp);
}

double keyToHeltz(double key) {
    if (key < 0.0) {
        return 1.0;
    }

    std::size_t offset = 300;
    int th = 900;
    double r = 1.0;
    while (th <= 14100) {
        if (key * 100 < th) {
            return r * centToHeltzTable.at(static_cast<std::size_t>(key * 100) + offset);
        }
        th += 1200;
        offset -= 1200;
        r *= 2.0;
    }

    return 1.0;
}

double timecentToSecond(double tc) {
    return std::exp2(tc / 1200.0);
}

double absoluteCentToHeltz(double ac) {
    return 8.176 * std::exp2(ac / 1200.0);
}

std::uint16_t joinBytes(std::uint8_t msb, std::uint8_t lsb) {
    return (static_cast<std::uint16_t>(msb) << 7) + static_cast<std::uint16_t>(lsb);
}
}
}
