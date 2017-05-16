/*
 * Copyright (c) 2016 CSIRO - Australia Telescope National Facility (ATNF)
 * 
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * PO Box 76, Epping NSW 1710, Australia
 * atnf-enquiries@csiro.au
 * 
 * This file is part of the ASKAP software distribution.
 * 
 * The ASKAP software distribution is free software: you can redistribute it
 * and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 * 
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
package askap.cp.manager.svcclients;

import org.apache.log4j.Logger;

import askap.interfaces.cp.ICPFuncTestReporterPrxHelper;
import askap.interfaces.cp.ICPFuncTestReporterPrx;
import askap.interfaces.schedblock.ObsState;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public class FuncTestReporterClient {

    /**
     * The interval (in seconds) between connection retry attempts.
     * This includes retrying connection to the Ice Locator service
     * of the TOS Data service.
     */
    private static final int RETRY_INTERVAL = 5;

    /**
     * Logger.
     */
    private static final Logger logger = Logger.getLogger(FuncTestReporterClient.class.getName());

	/** 
	 * The Ice client proxy.
	 */
    protected ICPFuncTestReporterPrx itsProxy = null;

    private static FuncTestReporterClient funcTestReporterClient = null;

	/**
	 * Instantiate a FuncTestReporterClient.
	 * 
	 * @param ic The Ice communicator.
	 * @param iceIdentity The Ice remote server identity string.
	 */
	private FuncTestReporterClient(Ice.Communicator ic, String iceIdentity) {
        logger.debug("Obtaining proxy to FuncTestReporterClient: " + iceIdentity);

        while (itsProxy == null) {
            final String baseWarn = " - will retry in " + RETRY_INTERVAL
                    + " seconds";
            try {
                Ice.ObjectPrx obj = ic.stringToProxy(iceIdentity);
                itsProxy = ICPFuncTestReporterPrxHelper.checkedCast(obj);
            } catch (Ice.ConnectionRefusedException e) {
                logger.warn("Connection refused" + baseWarn);
            } catch (Ice.NoEndpointException e) {
                logger.warn("No endpoint exception" + baseWarn);
            } catch (Ice.NotRegisteredException e) {
                logger.warn("Not registered exception" + baseWarn);
				logger.error(e);
            } catch (Ice.ConnectFailedException e) {
                logger.warn("Connect failed exception" + baseWarn);
            } catch (Ice.DNSException e) {
                logger.warn("DNS exception" + baseWarn);
            } catch (Ice.ObjectNotExistException e) {
                logger.warn("Object does not exist exception" + baseWarn);
				logger.error(e);
            } catch (Ice.SocketException e) {
                logger.warn("Socket exception" + baseWarn);
            }
            if (itsProxy == null) {
                try {
                    Thread.sleep(RETRY_INTERVAL * 1000);
                } catch (InterruptedException e) {
                    // In this rare case this might happen, faster polling is ok
                }
            }
        }
        logger.debug("Obtained proxy to FuncTestReporterClient");
    }

    public static synchronized void createFuncTestReporterClient(Ice.Communicator ic, String iceIdentity) {
	    if (funcTestReporterClient==null)
	        funcTestReporterClient = new FuncTestReporterClient(ic, iceIdentity);
    }

    public static synchronized FuncTestReporterClient getFuncTestReporterClient() {
	    return funcTestReporterClient;
    }

	/**
	 * Emit a scheduling block state change notification.
	 * @param sbid The scheduling block id.
	 * @param obsState The scheduling block state.
	 */
	public void notifySBStateChanged(long sbid, ObsState obsState) {
		itsProxy.sbStateChangedNotification(sbid, obsState);
	}

	/**
	 * Generic functional test feedback method where the only required
     * feedback is that a particular method has been called.
	 * @param name The method name to be reported back to the test driver.
	 */
	public void methodCalled(String name) {
		itsProxy.methodCalled(name);
	}
}
