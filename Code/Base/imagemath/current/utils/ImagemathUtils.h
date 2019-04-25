#ifndef IMAGEMATH_UTILS
#define IMAGEMATH_UTILS

#include <string>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/casa/Quanta/MVDirection.h>

casacore::MVDirection convertDir(const std::string &ra, const std::string &dec);

#endif

