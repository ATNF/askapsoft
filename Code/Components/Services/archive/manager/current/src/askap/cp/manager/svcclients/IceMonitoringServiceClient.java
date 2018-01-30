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
package askap.cp.manager.svcclients;

import askap.interfaces.monitoring.MonitorPoint;
import askap.interfaces.monitoring.MonitoringProviderPrx;
import askap.interfaces.monitoring.MonitoringProviderPrxHelper;
import askap.interfaces.schedblock.ISchedulingBlockServicePrx;
import askap.interfaces.schedblock.ISchedulingBlockServicePrxHelper;
import askap.interfaces.schedblock.NoSuchSchedulingBlockException;
import askap.interfaces.schedblock.ParameterException;
import askap.util.ParameterSet;
import org.apache.log4j.Logger;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;

public class IceMonitoringServiceClient implements IMonitoringServiceClient {
    /**
     * The interval (in seconds) between connection retry attempts.
     * This includes retrying connection to the Ice Locator service
     * of the TOS Data service.
     */
    private static final int RETRY_INTERVAL = 5;

    /**
     * Logger.
     */
    private static final Logger logger = Logger.getLogger(IceMonitoringServiceClient.class.getName());

    private MonitoringProviderPrx itsProxy = null;

    private final String iceIdentity;
    private final Ice.Communicator ic;

    private String[] pointNames = null;

    IceMonitoringServiceClient(Ice.Communicator ic, String iceIdentity, String[] pointNames) {
        this.ic = ic;
        this.iceIdentity = iceIdentity;
    }

    private void getProxy() {
        if (itsProxy==null) {
            Ice.ObjectPrx obj = ic.stringToProxy(iceIdentity);
            itsProxy = MonitoringProviderPrxHelper.checkedCast(obj);
            logger.info("Obtained proxy to MonitoringServiceClient: " + iceIdentity);
        }
    }

    @Override
    public List<MonitorPoint> get() {
        getProxy();
        MonitorPoint pointValues[] = itsProxy.get(this.pointNames);
        return Arrays.asList(pointValues);
    }
}
