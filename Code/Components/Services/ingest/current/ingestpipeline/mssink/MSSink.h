/// @file MSSink.h
///
/// @copyright (c) 2010 CSIRO
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
/// @author Ben Humphreys <ben.humphreys@csiro.au>

#ifndef ASKAP_CP_INGEST_MSSINK_H
#define ASKAP_CP_INGEST_MSSINK_H

// ASKAPsoft includes
#include "boost/scoped_ptr.hpp"
#include "boost/noncopyable.hpp"
#include "Common/ParameterSet.h"
#include "ms/MeasurementSets/MeasurementSet.h"
#include "casa/aips.h"
#include "casa/BasicSL.h"
#include "casa/Quanta.h"
#include "casa/Arrays/Vector.h"
#include "casa/Arrays/Matrix.h"
#include "cpcommon/VisChunk.h"

// Local package includes
#include "ingestpipeline/ITask.h"
#include "configuration/Configuration.h" // Includes all configuration attributes too

namespace askap {
namespace cp {
namespace ingest {

/// @brief A sink task for the central processor ingest pipeline which writes
/// the data out to a measurement set.
/// 
/// When constructing this class a measurement set is created, the default tables
/// are created and the ANTENNA, FEEDS, and OBSERVATION tables are populated
/// based on the "Configuration" instance passed to the constructor.
///
/// As observing takes place process() is called for each integration cycle. If
/// the VisChunk passed to process() is the first chunk for a new scan then rows
/// are added to the SPECTRAL WINDOW, POLARIZATION and DATA DESCRIPTION tables.
/// The visibilities and related data are also written into the main table.
class MSSink : public askap::cp::ingest::ITask,
               virtual public boost::noncopyable {
    public:
        /// @brief Constructor.
        /// @param[in] parset   the parameter set used to configure this task.
        /// @param[in] config   an object containing the system configuration.
        MSSink(const LOFAR::ParameterSet& parset,
                const Configuration& config);

        /// @brief Destructor.
        virtual ~MSSink();

        /// @brief Writes out the data in the VisChunk parameter to the 
        /// measurement set.
        ///
        /// @param[in,out] chunk    the instance of VisChunk to write out. Note
        ///                         the VisChunk pointed to by "chunk" nor the pointer
        ///                         itself are modified by this function.
        virtual void process(askap::cp::common::VisChunk::ShPtr& chunk);

        /// @brief should this task be executed for inactive ranks?
        /// @details If a particular rank is inactive, process method is
        /// not called unless this method returns true. This class has the
        /// following behavior.
        ///   - Returns true initially to allow collective operations if
        ///     number of ranks is more than 1.
        ///   - After the first call to process method, inactive ranks are
        ///     identified and false is returned for them.
        /// @return true, if process method should be called even if
        /// this rank is inactive (i.e. uninitialised chunk pointer
        /// will be passed to process method).
        virtual bool isAlwaysActive() const;

    private:
        /// @brief initialise the measurement set
        /// @details In the serial mode we run initialisation in the constructor.
        /// However, in the parallel mode it is handy to initialise the measurement set
        /// upon the first call to process method (as this is the only way to automatically
        /// deduce which ranks are active and which are not; the alternative design would be
        /// to specify this information in parset, but this seems to be unnecessary). 
        /// This method encapsulates all required initialisation actions.
        void initialise();


        /// @brief add non-standard column to POINTING table
        /// @details We use 3 non-standard columns to capture
        /// actual pointing on all three axes. This method creates one such
        /// column.
        /// @param[in] name column name
        /// @param[in] description text description
        void addNonStandardPointingColumn(const std::string &name, 
                  const std::string &description);   

        /// @brief make substitution in the file name
        /// @details To simplify configuring the pipeline for different purposes certain
        /// expressions are recognised and substituted by this methiod
        /// %w is replaced by the rank, %d is replaced by the date (in YYYY-MM-DD format),
        /// %t is replaced by the time (in HHMMSS format). Note, both date and time are 
        /// obtained on the rank zero and then broadcast to other ranks, unless in the standalone
        /// mode.
        /// @param[in] in input file name (may contain patterns to substitute)
        /// @return file name with patterns substituted
        std::string substituteFileName(const std::string &in) const;

        /// @brief make two-character string
        /// @details Helper method to convert unsigned integer into a 2-character string.
        /// It is used to form the file name with date and time.
        /// @param[in] in input number
        /// @return two-element string
        static std::string makeTwoElementString(const casa::uInt in);
          

        // Initialises the ANTENNA table
        void initAntennas(void);

        // Initialises the FEED table
        void initFeeds(const FeedConfig& feeds, const casa::Int antennaID);

        // Initialises the OBSERVATION table
        void initObs(void);

        // Create the measurement set
        void create(void);

        // Add observation table row
        casa::Int addObs(const casa::String& telescope,
                const casa::String& observer,
                const double obsStartTime,
                const double obsEndTime);

        // Add field table row
        casa::Int addField(const casa::String& fieldName,
                const casa::MDirection& fieldDirection,
                const casa::String& calCode);

        // Add feeds table rows
        void addFeeds(const casa::Int antennaID,
                const casa::Vector<double>& x,
                const casa::Vector<double>& y,
                const casa::Vector<casa::String>& polType);

        // Add antenna table row
        casa::Int addAntenna(const casa::String& station,
                const casa::Vector<double>& antXYZ,
                const casa::String& name,
                const casa::String& mount,
                const casa::Double& dishDiameter);

        // Add entries from the VisChunk to the pointint table
        void addPointingRows(const askap::cp::common::VisChunk& chunk);

        // Add data description table row
        casa::Int addDataDesc(const casa::Int spwId, const casa::Int polId);

        // Add spectral window table row
        casa::Int addSpectralWindow(const casa::String& name,
                const int nChan,
                const casa::Quantity& startFreq,
                const casa::Quantity& freqInc);

        // Add polarisation table row
        casa::Int addPolarisation(const casa::Vector<casa::Stokes::StokesTypes>& stokesTypes);

        // Find or add a FIELD table entry
        casa::Int findOrAddField(const askap::cp::common::VisChunk::ShPtr chunk);

        // Find or add a DATA DESCRIPTION (including SPECTRAL INDEX and POLARIZATION)
        // table entry for the provided scan index number.
        casa::Int findOrAddDataDesc(askap::cp::common::VisChunk::ShPtr chunk);

        // Compares the given row in the spectral window table with the spectral window
        // setup as defined in the Scan.
        bool isSpectralWindowRowEqual(askap::cp::common::VisChunk::ShPtr chunk,
                const casa::uInt row) const;

        // Compares the given row in the polarisation table with the polarisation
        // setup as defined in the Scan.
        bool isPolarisationRowEqual(askap::cp::common::VisChunk::ShPtr chunk,
                const casa::uInt row) const;

        // Generate any monitoring data from the "chunk" and submit to the
        // MonitoringSingleton
        void submitMonitoringPoints(askap::cp::common::VisChunk::ShPtr chunk);

        // Helper function to compare MDirections
        static bool equal(const casa::MDirection &dir1, const casa::MDirection &dir2);


        /// @brief helper method to obtain stream sequence number
        /// @details It does counting of active ranks across the whole rank space.
        /// @param[in] isActive true if this rank is active, false otherwise
        /// @return sequence number of the stream handled by this rank or -1 if it is
        /// not active.
        /// @note The method uses MPI collective calls and should be executed by all ranks,
        /// including inactive ones.
        int countActiveRanks(bool isActive) const;

        // Parameter set
        const LOFAR::ParameterSet itsParset;

        // Configuration object
        const Configuration itsConfig;

        // True if the POINTING table should be written
        bool itsPointingTableEnabled;

        // The index number of the scan for the previous VisChunk. Some things
        // (such as spectral window or field) are allowed to change from scan
        // to scan, this allows a new scan to be detected
        casa::Int itsPreviousScanIndex;

        // The current field row. This is cached until the scan index is
        // incremented
        casa::Int itsFieldRow;

        // The current data description row. This is cached until the scan
        // index is incremented
        casa::Int itsDataDescRow;

        // Measurement set
        boost::scoped_ptr<casa::MeasurementSet> itsMs;

        /// @brief sequence number of the stream
        /// @details We may have more MPI ranks available than the number of
        /// parallel streams (need more receiving processes than number crunchers)
        /// This field is filled inside countActiveRanks method based on whether it got
        /// an empty shared pointer or not. This field is not used in the serial mode and
        /// is always intialised with 0. It is set to -1 for ranks which are not active.
        int itsStreamNumber;

        /// @brief name of the MS to write
        /// @details Each active rank writes file under unique name which is stored here
        std::string itsFileName;
};

}
}
}

#endif
