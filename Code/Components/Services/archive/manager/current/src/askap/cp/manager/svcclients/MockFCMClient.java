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
 */
package askap.cp.manager.svcclients;

import java.io.IOException;
import org.apache.log4j.Logger;
import askap.util.ParameterSet;


/**
 * This class provides a mock implementation of IFCMClient. It is instantiated
 * with a parameter set that will be returned when get() is called.
 */
public class MockFCMClient implements IFCMClient {

    /**
     * Logger.
     */
    private static final Logger logger = Logger.getLogger(MockFCMClient.class.getName());

    /**
     * The configuration that will be returned from get();
     */
    private ParameterSet itsData;

    /**
     * Constructor.
     *
     * @param filename  the filename of the file containing the parameter set
     *                  that will be provided whenever a client calls
     *                  get() on objects of this class
     */
    public MockFCMClient(String filename) {
        try {
            itsData = new ParameterSet(
					askap.util.Path.expandvars(filename));
        } catch (IOException e) {
			logger.error(e);
        }
    }

    /**
     * @see askap.cp.manager.svcclients.IFCMClient#get()
     */
    @Override
    public ParameterSet get() {
        return itsData;
    }
	
    /**
     * @see askap.cp.manager.svcclients.IFCMClient#get()
     */
    public ParameterSet get(String key) {
		return itsData.subset(key);
	}
}
