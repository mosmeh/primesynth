#include "conv.h"
#include <cmath>

namespace primasynth {

double centibelToRatio(double cb) {
    return std::pow(10.0, -cb / 100.0);
}

double keyToHeltz(double key) {
    return 440.0 * std::exp2((key - 69.0) / 12.0);
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

