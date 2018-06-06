#pragma once
#include "sfspec.h"

namespace primasynth {

void initializeConversionTables();
double centibelToRatio(double cb);
double keyToHeltz(double key);
double timecentToSecond(double tc);
double absoluteCentToHeltz(double ac);
std::uint16_t joinBytes(std::uint8_t msb, std::uint8_t lsb);

}

