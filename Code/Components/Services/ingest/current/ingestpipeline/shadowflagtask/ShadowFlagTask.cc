/// @file ShadowFlagTask.cc
///
/// @copyright (c) 2013 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

// Include own header file first
#include "ingestpipeline/shadowflagtask/ShadowFlagTask.h"

// Include package level header file
#include "askap_cpingest.h"

// ASKAPsoft includes
#include "askap/AskapLogging.h"
#include "askap/AskapError.h"
#include "askap/AskapUtil.h"

ASKAP_LOGGER(logger, ".ShadowFlagTask");

using namespace askap;
using namespace askap::cp::common;
using namespace askap::cp::ingest;


/// @brief Constructor
ShadowFlagTask::ShadowFlagTask(const LOFAR::ParameterSet &parset, const Configuration &config) : 
     itsDishDiameter(parset.getFloat("dish_diameter", 12)), itsDryRun(parset.getBool("dry_run", false)),
     itsNumberOfBeams(-1)
{
   ASKAPLOG_DEBUG_STR(logger, "Constructor");
   itsAntennaNames.reserve(config.antennas().size());
   for (vector<Antenna>::const_iterator ci = config.antennas().begin(); ci != config.antennas().end(); ++ci) {
        itsAntennaNames.push_back(ci->name());
        // could've got the actual diameter from the configuration here, but we'd have to support
        // heterogeneous case then or ensure all diameters are the same.
   }
}

/// @brief destructor
ShadowFlagTask::~ShadowFlagTask()
{
}

/// @brief Flag data in the specified VisChunk if necessary.
/// @details This method applies static scaling factors to correct for FFB ripple
///
/// @param[in,out] chunk  the instance of VisChunk for which the
///                       scaling factors will be applied.
void ShadowFlagTask::process(askap::cp::common::VisChunk::ShPtr& chunk)
{
   ASKAPDEBUGASSERT(chunk);
   if (itsNumberOfBeams < 0) {
       std::set<casacore::uInt> beamIDs(chunk->beam1().begin(),chunk->beam1().end());
       itsNumberOfBeams = static_cast<int>(beamIDs.size());
       ASKAPCHECK(itsNumberOfBeams > 0, "Data chunk received on the first iteration seems to be empty");
   }
   // first build a set of shadowed antennas
   std::set<casacore::uInt> shadowedAntennasThisCycle;
   casacore::Vector<casacore::RigidVector<casacore::Double, 3> > &uvw = chunk->uvw();
   for (casacore::uInt row = 0; row < chunk->nRow(); ++row) {
        if (chunk->antenna1()[row] != chunk->antenna2()[row]) {
            const casacore::RigidVector<casacore::Double, 3> thisRowUVW = uvw[row];
            const double projectedSeparation = sqrt(thisRowUVW(0)*thisRowUVW(0) + thisRowUVW(1)*thisRowUVW(1));
            const double baselineLength = sqrt(thisRowUVW(2) * thisRowUVW(2) + projectedSeparation * projectedSeparation);
            if (baselineLength < 1e-6) {
                // this is the feature of TOS-calculated uvw that they're zero for completely flagged baselines
                // check that this baseline is indeed flagged. Note, autocorrelations are already excluded by the if-statement above
                bool oneUnflagged = false;
                casacore::Matrix<casacore::Bool> thisRowFlags = chunk->flag().yzPlane(row);
                // it may be faster to do it via flattened array as we don't care
                // which element is where, but for now leave the code more readable
                for (casacore::uInt chan = 0; chan < thisRowFlags.nrow(); ++chan) {
                     for (casacore::uInt pol = 0; pol < thisRowFlags.ncolumn(); ++pol) {
                          if (!thisRowFlags(chan,pol)) {
                              oneUnflagged = true;
                              break;
                          }
                     }
                }
                ASKAPCHECK(!oneUnflagged, "Inconsistency in uvw is detected: they are missing or equal to zero for unflagged "<<chunk->antenna1()[row]<<" - "<<
                           chunk->antenna2()[row]<<" baseline");
                continue;
            }
            if (projectedSeparation < itsDishDiameter) {
                if (thisRowUVW(2) < 0.) {
                    // antenna 1 is behind antenna 2 (it's second to first notation)
                    shadowedAntennasThisCycle.insert(chunk->antenna1()[row]);
                } else {
                    shadowedAntennasThisCycle.insert(chunk->antenna2()[row]);
                }
                ASKAPCHECK(sqrt(thisRowUVW(2) * thisRowUVW(2) + projectedSeparation * projectedSeparation) > 12., 
                           "Antennas should've collided in this configuration or there is a logic bug, or it's not ASKAP");
            }
        }
   }
   const bool logAtHigherPriority = (itsNumberOfBeams > 1) || (chunk->nRow() > 0 ? chunk->beam1()[0] == 0 : false);
   // now, check on changes in the list of flagged antennas for reporting
   for (std::set<casacore::uInt>::const_iterator ci = shadowedAntennasThisCycle.begin(); 
                      ci != shadowedAntennasThisCycle.end(); ++ci) {
        if (itsShadowedAntennas.find(*ci) == itsShadowedAntennas.end()) {
            ASKAPDEBUGASSERT(*ci < itsAntennaNames.size());
            if (logAtHigherPriority) {
                ASKAPLOG_WARN_STR(logger, "Antenna "<<itsAntennaNames[*ci]<<" (id="<<*ci<<
                     ") is now shadowed, corresponding baselines will be flagged until further notice");
            } else {
                ASKAPLOG_DEBUG_STR(logger, "Antenna "<<itsAntennaNames[*ci]<<" (id="<<*ci<<
                     ") is now shadowed, corresponding baselines will be flagged until further notice");
            }
        }
   }

   for (std::set<casacore::uInt>::const_iterator ci = itsShadowedAntennas.begin(); 
                      ci != itsShadowedAntennas.end(); ++ci) {
        if (shadowedAntennasThisCycle.find(*ci) == shadowedAntennasThisCycle.end()) {
            ASKAPDEBUGASSERT(*ci < itsAntennaNames.size());
            if (logAtHigherPriority) {
                ASKAPLOG_WARN_STR(logger, "Antenna "<<itsAntennaNames[*ci]<<" (id="<<*ci<<
                      ") is no longer shadowed");
            } else {
                ASKAPLOG_DEBUG_STR(logger, "Antenna "<<itsAntennaNames[*ci]<<" (id="<<*ci<<
                      ") is no longer shadowed");
            }
        }
   }
   itsShadowedAntennas = shadowedAntennasThisCycle;
   
   // now flag affected baselines
   if (!itsDryRun && itsShadowedAntennas.size() > 0) {
       for (casacore::uInt row = 0; row < chunk->nRow(); ++row) {
            if ((itsShadowedAntennas.find(chunk->antenna2()[row]) != itsShadowedAntennas.end()) || 
                (itsShadowedAntennas.find(chunk->antenna1()[row]) != itsShadowedAntennas.end())) {
                // flag this row
                casacore::Matrix<casacore::Bool> thisFlagRow = chunk->flag().yzPlane(row);
                thisFlagRow.set(casacore::True); 
            }
       }
   }
}

