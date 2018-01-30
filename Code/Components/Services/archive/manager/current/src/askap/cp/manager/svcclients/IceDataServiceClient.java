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

import askap.interfaces.schedblock.ParameterException;
import org.apache.log4j.Logger;

import askap.interfaces.schedblock.ISchedulingBlockServicePrx;
import askap.interfaces.schedblock.ISchedulingBlockServicePrxHelper;
import askap.interfaces.schedblock.NoSuchSchedulingBlockException;
import askap.util.ParameterSet;

import java.util.Map;

public class IceDataServiceClient implements IDataServiceClient {
    /**
     * Logger.
     */
    private static final Logger logger = Logger.getLogger(IceDataServiceClient.class.getName());
    private final String iceIdentity;
    private final Ice.Communicator ic;

    private ISchedulingBlockServicePrx itsProxy = null;



    IceDataServiceClient(Ice.Communicator ic, String iceIdentity) {
        this.ic = ic;
        this.iceIdentity = iceIdentity;
    }

    void getProxy() {
        logger.info("Obtaining proxy to DataServiceClient");

        Ice.ObjectPrx obj = ic.stringToProxy(iceIdentity);
        itsProxy = ISchedulingBlockServicePrxHelper.checkedCast(obj);
        logger.info("Obtained proxy to DataServiceClient");
    }

    /**
     * @see askap.cp.manager.svcclients.IDataServiceClient#getObsParameters(long)
     */
    public ParameterSet getObsParameters(long sbid)
            throws NoSuchSchedulingBlockException {
        getProxy();
        return new ParameterSet(itsProxy.getObsParameters(sbid));
    }

    @Override
    public void setObsVariables(long sbid, Map<String, String> obsVars) throws NoSuchSchedulingBlockException, ParameterException {
        getProxy();
        itsProxy.setObsVariables(sbid, obsVars);
    }

// TODO: Implement getState and transition on ISchedulingBlockServicePrx
}
