
#include "ImagemathUtils.h"

#include <string>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/casa/Quanta/MVDirection.h>

casa::MVDirection convertDir(const std::string &ra, const std::string &dec) {
    casa::Quantity tmpra,tmpdec;
    casa::Quantity::read(tmpra, ra);
    casa::Quantity::read(tmpdec,dec);
    return casa::MVDirection(tmpra,tmpdec);
}

