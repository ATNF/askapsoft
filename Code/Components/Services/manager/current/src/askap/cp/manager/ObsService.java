/**
 *  Copyright (c) 2011 CSIRO - Australia Telescope National Facility (ATNF)
 *
 *  Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 *  PO Box 76, Epping NSW 1710, Australia
 *  atnf-enquiries@csiro.au
 *
 *  This file is part of the ASKAP software distribution.
 *
 *  The ASKAP software distribution is free software: you can redistribute it
 *  and/or modify it under the terms of the GNU General Public License as
 *  published by the Free Software Foundation; either version 2 of the License,
 *  or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 * @author Ben Humphreys <ben.humphreys@csiro.au>
 */
package askap.cp.manager;

import org.apache.log4j.Logger;

import Ice.Current;
import askap.interfaces.cp._ICPObsServiceDisp;
import askap.util.ParameterSet;
import askap.cp.manager.ingest.AbstractIngestManager;
import askap.cp.manager.ingest.TestIngestManager;
import askap.cp.manager.ingest.ProcessIngestManager;
import askap.cp.manager.svcclients.FuncTestReporterClient;
import askap.cp.manager.svcclients.IFCMClient;

/**
 * Implements the Central Processor Observation Service
 */
public class ObsService extends _ICPObsServiceDisp {

    /**
     * Logger.
     */
    private static final Logger logger = Logger.getLogger(ObsService.class.getName());

    /**
     * Facility Configuration Manager client wrapper instance
     */
    private final IFCMClient itsFCM;

    /**
     * TOS Data Service client wrapper instance
     */
    // private final IDataServiceClient itsDataService;

    /**
     * Ingest Pipeline Controller
     */
    private final AbstractIngestManager itsIngestManager;

	/**
	 * Factory method for the Observation Service. Implemented so that the 
	 * constructor uses dependency injection for easier testing with mocks.
	 * 
     * @param ic        the Ice Communicator that will be used for communication
     *                  with the external services (e.g. FCM)
     * @param parset    the parameter set this program was configured with
	 * @param fcm 		The FCM client
	 * @return 			The ObsService instance.
	 */
	public static final ObsService Create(
			Ice.Communicator ic,
			ParameterSet parset,
			IFCMClient fcm) {
		AbstractIngestManager itsIngestManager = null;

        logger.debug("ObsService factory");

        // Instantiate real or mock data service interface
        /*
		 * boolean mockdatasvc = parset.getBoolean("dataservice.mock", false);
		 * if (mockdatasvc) { itsDataService = new
		 * MockDataServiceClient(parset.getString("dataservice.mock.filename"));
		 * } else { String identity =
		 * parset.getString("dataservice.ice.identity"); if (identity == null) {
		 * throw new
		 * RuntimeException("Parameter 'dataservice.ice.identity' not found"); }
		 * itsDataService = new IceDataServiceClient(ic, identity); }
		 */

        // Create Ingest Manager
        String managertype = parset.getString("ingest.managertype", "process");
        if (managertype.equalsIgnoreCase("process")) {  
			logger.debug("ObsService factory: creating ingest manager");
            itsIngestManager = new ProcessIngestManager(parset);
        } else if (managertype.equalsIgnoreCase("test")) {
			logger.debug("ObsService factory: creating mock ingest manager");
			FuncTestReporterClient client = new FuncTestReporterClient(
				ic,
				parset.getString("cpfunctestreporter.identity"));
            itsIngestManager = new TestIngestManager(parset, client);
        } else {
            throw new RuntimeException("Unknown ingest manager type: "
                    + managertype);
        }

		logger.debug("ObsService factory: instantiating ObsService instance");
		return new ObsService(fcm, itsIngestManager);
	}

    /**
     * Constructor
	 * @param itsFCM 	Facility Configuration Manager client wrapper instance
	 * @param itsIngestManager  Ingest Pipeline Controller
     */
    public ObsService(IFCMClient itsFCM, AbstractIngestManager itsIngestManager) {
        super();
        logger.debug("ObsService constructor");
		this.itsFCM = itsFCM;
		this.itsIngestManager = itsIngestManager;
    }

    /**
     * See slice interface for API documentation
	 * @throws askap.interfaces.cp.NoSuchSchedulingBlockException
	 * @throws askap.interfaces.cp.AlreadyRunningException
	 * @throws askap.interfaces.cp.PipelineStartException
     */
    @Override
    public void startObs(long sbid, Current curr)
            throws askap.interfaces.cp.NoSuchSchedulingBlockException,
            askap.interfaces.cp.AlreadyRunningException,
            askap.interfaces.cp.PipelineStartException {
        try {
            logger.info("Executing scheduling block " + sbid);

            logger.debug("Obtaining FCM parameters");
            ParameterSet fc = itsFCM.get();
			logger.trace("parset keys: " + fc.keys());
            if (fc == null || fc.isEmpty()) {
                logger.error("Error fetching FCM data");
                throw new askap.interfaces.cp.PipelineStartException(
                        "Error fetching FCM data");
            }

			/*
			 * logger.debug("Obtaining observation parameters"); ParameterSet
			 * obsParams;
			 * 
			 * try { obsParams = itsDataService.getObsParameters(sbid); } catch
			 * (askap.interfaces.schedblock.NoSuchSchedulingBlockException e) {
			 * String msg = "Scheduling block " + sbid + " does not exist";
			 * throw new
			 * askap.interfaces.cp.NoSuchSchedulingBlockException(msg); }
			 */

            // BLOCKING: Will block until the ingest pipeline starts, or
            // an error occurs
            itsIngestManager.startIngest(fc, sbid);
        } catch (askap.interfaces.cp.AlreadyRunningException e) {
            logger.warn("startObs() - AlreadyRunningException: " + e.reason);
            throw e;
        } catch (askap.interfaces.cp.PipelineStartException e) {
            logger.warn("startObs() - PipelineStartException: " + e.reason);
            throw e;
        } catch (Exception e) {
            logger.error("startObs() - Unexpected exception: " + e.getMessage());
            logger.error(e.getStackTrace());
            throw new askap.interfaces.cp.PipelineStartException(e.getMessage());
        }
    }

    /**
     * See slice interface for API documentation
     */
    @Override
    public void abortObs(Current curr) throws Ice.UnknownException {
        try {
            // Blocking (until aborted)
            itsIngestManager.abortIngest();
        } catch (Exception e) {
            logger.error("abortObs() - Unexpected exception: " + e.getMessage());
            logger.error(e.getStackTrace());
            throw new Ice.UnknownException(e.getMessage());
        }
    }

    /**
     * See slice interface for API documentation
     */
    @Override
    public boolean waitObs(long timeout, Current curr) throws Ice.UnknownException {
        try {
            // TODO: This is the point at which cpman should switch to active mode
			logger.debug("timeout = " + timeout);
            return itsIngestManager.waitIngest(timeout);
        } catch (Exception e) {
            logger.error("waitObs() - Unexpected exception: " + e.getMessage());
            logger.error(e.getStackTrace());
            throw new Ice.UnknownException(e.getMessage());
        }
    }

    /**
     * See slice interface for API documentation
     */
    @Override
    public String getServiceVersion(Current curr) throws Ice.UnknownException {
        try {
            Package p = this.getClass().getPackage();
            return p.getImplementationVersion();
        } catch (Exception e) {
            logger.error("waitObs() - Unexpected exception: " + e.getMessage());
            logger.error(e.getStackTrace());
            throw new Ice.UnknownException(e.getMessage());
        }
    }
}
