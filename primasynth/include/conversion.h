#pragma once
#include <cstdint>

namespace primasynth {
namespace conv {
void initialize();
double attenuationToAmplitude(double atten);
double amplitudeToAttenuation(double amp);
double keyToHeltz(double key);
double timecentToSecond(double tc);
double absoluteCentToHeltz(double ac);
std::uint16_t joinBytes(std::uint8_t msb, std::uint8_t lsb);
}
}
