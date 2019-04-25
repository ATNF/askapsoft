

#include <string>
#include <casacore/casa/Quanta/Quantum.h>
#include <casacore/casa/Quanta/MVDirection.h>

#include <utils/ImagemathUtils.h>

casacore::MVDirection convertDir(const std::string &ra, const std::string &dec) {
    casacore::Quantity tmpra,tmpdec;
    casacore::Quantity::read(tmpra, ra);
    casacore::Quantity::read(tmpdec,dec);
    return casacore::MVDirection(tmpra,tmpdec);
}

