/// @file DaliugeApplication.h
/// @brief Base class for Daliuge applications
///
/// @copyright (c) 2017 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au>

#ifndef ASKAP_DALIUGE_APPLICATION_H
#define ASKAP_DALIUGE_APPLICATION_H

// System includes
#include <string>
//ASKAPSoft includes
#include <boost/shared_ptr.hpp>
// daliugue includes
#include "dlg_app.h"

namespace askap {

    /// @brief Daliuge application class.
    /// This class encapsulates the functions required of a daliuge application
    /// as specified in dlg_app.h then exposes them as C functions


    class DaliugeApplication {
        public:

            /// The application name
            /// Not sure this will work - but we can try.
            // I think I need this because I need to know which app to instantiate
            // the app has no persistence between calls .... maybe it should


            /// Shared pointer definition
            typedef boost::shared_ptr<DaliugeApplication> ShPtr;

            /// Constructor
            DaliugeApplication();

            /// Destructor
            virtual ~DaliugeApplication();

            /// This has to be static as we need to access it in the register even
            /// if there is not instantiated class.

            static ShPtr createDaliugeApplication(const std::string& theAppName);

            /// This function is implemented by sub-classes. i.e. The users of
            /// this class.


            virtual int init(dlg_app_info *app, const char ***arguments) = 0;

            virtual int run(dlg_app_info *app) = 0;

            virtual void data_written(dlg_app_info *app, const char *uid,
                const char *data, size_t n) = 0;

            virtual void drop_completed(dlg_app_info *app, const char *uid,
                drop_status status) = 0 ;

    };

} // End namespace askap



#endif
