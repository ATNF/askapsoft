/// @file CliDev.h
/// @brief Defines some additional developer-only command-line utilities.
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
/// @author Daniel Collins <daniel.collins@csiro.au>

#ifndef ASKAP_CP_SMS_CLI_DEV_H
#define ASKAP_CP_SMS_CLI_DEV_H

// Include package level header file
#include <askap_skymodel.h>

// ASKAPsoft includes
#include "GlobalSkyModel.h"
#include "Cli.h"

namespace askap {
namespace cp {
namespace sms {

class CliDev : public Cli {
    public:

        CliDev() : Cli() {}
        virtual ~CliDev() {}

    protected:

        virtual void doAddParameters();
        virtual int doCommandDispatch();

    private:

        int createSchema();
        int generateRandomComponents(int64_t componentCount);
        void populateRandomComponents(
            ComponentList& components,
            int64_t sbid);
};

};
};
};

#endif
