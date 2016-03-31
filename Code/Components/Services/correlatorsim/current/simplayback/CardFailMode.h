/// @file CardFailMode.h
///
/// @copyright (c) 2016 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Paulus Lahur <paulus.lahur@csiro.au>

#ifndef ASKAP_CP_SIMPLAYBACK_CARDFAILMODE_H
#define ASKAP_CP_SIMPLAYBACK_CARDFAILMODE_H

// System includes
#include <stdint.h>


namespace askap {
namespace cp {

/// @brief Failure modes for a card in Correlator Simulator.
class CardFailMode {

    public:

		/// Constructor
		CardFailMode();

        /// Destructor
        //virtual ~CardFailMode();

		/// Print all mode status
		void print();


		/// True if the card fails in any way (default: false)
		bool fail;

		/// The card misses transmission at a given cycle
		/// 0 (default): no missing transmission
		uint32_t miss;

		// TODO
		// The card has mismatch in its BAT
		// The amount of mismatch is given below (early is positive)
		// 0 (default): no mismatch
		// int32_t batMismatch;
		//
		// Add more failure modes here
		//

    //private:

};

};
};
#endif
