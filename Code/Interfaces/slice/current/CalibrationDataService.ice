// @file CalibrationDataService.ice
//
// @copyright (c) 2011 CSIRO
// Australia Telescope National Facility (ATNF)
// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
// PO Box 76, Epping NSW 1710, Australia
// atnf-enquiries@csiro.au
//
// This file is part of the ASKAP software distribution.
//
// The ASKAP software distribution is free software: you can redistribute it
// and/or modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the License,
// or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

#ifndef ASKAP_CALIBRATION_DATA_SERVICE_ICE
#define ASKAP_CALIBRATION_DATA_SERVICE_ICE

#include <CommonTypes.ice>
#include <CalibrationParameters.ice>
#include <IService.ice>

module askap
{
module interfaces
{
module caldataservice
{
    /**
     * This exception is thrown when a solution id is specified but does not exist,
     * when is expected to.
     **/
    exception UnknownSolutionIdException extends askap::interfaces::AskapIceException
    {
    };

    /**
     * This exception is thrown by add methods if solution of the same type already exists
     **/
    exception AlreadyExists extends askap::interfaces::AskapIceException
    {
    };


    /**
     * Interface to the Calibration Data Service.
     **/
    interface ICalibrationDataService extends askap::interfaces::services::IService
    {
        /**
         * Get the new solution ID
         *
         * Creates a new database entry for calibration solution(s) and returns unique id
         * which can be used together with add or update methods
         * @return the unique ID of the new entry to be populated later
         */
        long newSolutionID();
        

        // the following methods just store the solution as is

        /**
         * Add a new time tagged gains solution to the calibration
         * data service.
         * 
         * @param id entry to add the solution to
         * @param solution  new time tagged gains solution.
         * @note An exception is thrown if gain solution has already been added for this id
         */
        void addGainsSolution(long id, askap::interfaces::calparams::TimeTaggedGainSolution solution)
            throws AlreadyExists;

        /**
         * Add a new time tagged bandpass solution to the calibration
         * data service.
         * 
         * @param id entry to add the solution to
         * @param solution  new time tagged bandpass solution.
         * @note An exception is thrown if bandpass solution has already been added for this id
         */
        void addBandpassSolution(long id, askap::interfaces::calparams::TimeTaggedBandpassSolution solution)
            throws AlreadyExists;

        /**
         * Add a new time tagged leakage solution to the calibration
         * data service.
         * 
         * @param id entry to add the solution to
         * @param solution  new time tagged leakage solution.
         * @note An exception is thrown if leakage solution has already been added for this id
         */
        void addLeakageSolution(long id, askap::interfaces::calparams::TimeTaggedLeakageSolution solution)
            throws AlreadyExists;

        // the following methods treat the given solution as an update relative to the most recent entry
        // which has a solution of the appropriate type. We can probably skip implementation of these
        // methods for version one, but this is what the calibration pipeline will be using.

        // one complication is that solutions may not always contain same antennas/beams, but it requires
        // more thought

        /**
         * Merge in a new time tagged gains solution with the latest gains and store it
         * in the calibration data service. If no previous solutions are present, store as is.
         * 
         * @param id entry to attach the resulting solution to
         * @param solution  new time tagged gains solution treated as an update
         * @note An exception is thrown if gain solution has already been added for this id
         */
        void adjustGains(long id, askap::interfaces::calparams::TimeTaggedGainSolution solution)
            throws AlreadyExists;

        /**
         * Merge in a new time tagged bandpass solution with the latest bandpass and store the 
         * result in the calibration data service. If no previous solutions are present, store as is.
         * 
         * @param id entry to attach the resulting solution to
         * @param solution  new time tagged bandpass solution.
         * @note An exception is thrown if bandpass solution has already been added for this id
         */
        void adjustBandpass(long id, askap::interfaces::calparams::TimeTaggedBandpassSolution solution)
            throws AlreadyExists;

        /**
         * Merge in a new time tagged leakage solution with the latest leakage solution and store 
         * the result to the calibration data service (as a new solution)
         * 
         * @param id entry to add the solution to
         * @param solution  new time tagged leakage solution.
         * @note An exception is thrown if leakage solution has already been added for this id
         */
        void adjustLeakages(long id, askap::interfaces::calparams::TimeTaggedLeakageSolution solution)
            throws AlreadyExists;

        // methods to check what kind of calibration information is attached to the given id

        /**
         * Check that the gain solution is present.
         * @param id entry to check
         * @return true, if the given id has a gain solution.
         */
        bool hasGainSolution(long id);

        /**
         * Check that the leakage solution is present.
         * @param id entry to check
         * Obtain the ID of the current/optimum leakage solution.
         * @return true, if the given id has a leakage solution.
         */
        bool hasLeakageSolution(long id);

        /**
         * Check that the bandpass solution is present.
         * @param id entry to check
         * @return true, if the given id has a bandpass solution.
         */
        bool hasBandpassSolution(long id);

        /**
         * Obtain the most recent solution ID in the database
         * @note Solution ID corresponds to one entry in the database resembling a row
         * in the calibration table. It may contain gain, bandpass and leakage solutions
         * in any combinations (i.e. just gains, gains+bandpasses, all three, etc)
         * 
         * exception is thrown if the database is empty or any other fatal error
         * @return ID of the last entry
         */
        long getLatestSolutionID()
            throws UnknownSolutionIdException;

        // the following two methods are intended for monitoring/visualisation
        // they are not used by ingest/calibration pipeline and can be further discussed

        /**
         * Obtain smallest solution ID corresponding to the time >= the given timestamp
         * @param timestamp absolute time given as MJD in the UTC frame (same as timestamp
         *                  in solutions - can be compared directly)
         * @return solution ID 
         * @note gain, bandpass and leakage solutions corresponding to one solution ID
         *       can have different timestamps. Use the greatest for comparison.
         * if all the timestamps in the stored solutions are less than the given timestamp,
         * this method is equivalent to getLatestSolutionID(). 
         */
        long getUpperBoundID(double timestamp)
            throws UnknownSolutionIdException;

        /**
         * Obtain largest solution ID corresponding to the time <= the given timestamp
         * @param timestamp absolute time given as MJD in the UTC frame (same as timestamp
         *                  in solutions - can be compared directly)
         * @return solution ID 
         * @note gain, bandpass and leakage solutions corresponding to one solution ID
         *       can have different timestamps. Use the smallest for comparison.
         * if all the timestamps in the stored solutions are greater than the given timestamp,
         * this method should return zero. 
         */
        long getLowerBoundID(double timestamp)
            throws UnknownSolutionIdException;

        // access methods

        /**
         * Get a gain solution.
         * @param id    id of the gain solution to obtain.
         * @return the gain solution.
         *
         * @throws UnknownSolutionIdException   the id parameter does not refer to
         *                                      a known solution.
         */
        askap::interfaces::calparams::TimeTaggedGainSolution getGainSolution(long id)
            throws UnknownSolutionIdException;

        /**
         * Get a leakage solution.
         * @param id    id of the leakage solution to obtain.
         * @return the leakage solution.
         *
         * @throws UnknownSolutionIdException   the id parameter does not refer to
         *                                      a known solution.
         */
        askap::interfaces::calparams::TimeTaggedLeakageSolution getLeakageSolution(long id)
            throws UnknownSolutionIdException;

        /**
         * Get a bandpass solution.
         * @param id    id of the bandpass solution to obtain.
         * @return the bandpass solution.
         *
         * @throws UnknownSolutionIdException   the id parameter does not refer to
         *                                      a known solution.
         */
        askap::interfaces::calparams::TimeTaggedBandpassSolution getBandpassSolution(long id)
            throws UnknownSolutionIdException;
    };

};
};
};

#endif
