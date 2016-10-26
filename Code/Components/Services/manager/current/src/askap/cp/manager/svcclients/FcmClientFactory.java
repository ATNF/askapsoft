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

import Ice.Communicator;
import askap.util.ParameterSet;
import org.apache.log4j.Logger;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public final class FcmClientFactory {

	private static final Logger logger = Logger.getLogger(FcmClientFactory.class.getName());

	/**
	 * Factory method for FCM clients
	 *
	 * @param config: The parameter set.
	 * @param communicator: The Ice communicator.
	 * @return IFCMClient
	 */
	public static IFCMClient getInstance(
			ParameterSet config,
			Communicator communicator) {

		IFCMClient fcm = null;

		// Instantiate real or mock FCM
		boolean mockfcm = config.getBoolean("fcm.mock", false);
		if (mockfcm) {
			logger.debug("ObsService factory: creating mock FCM");
			fcm = new MockFCMClient(config.getString("fcm.mock.filename"));
		} else {
			logger.debug("ObsService factory: creating FCM");
			String identity = config.getString("fcm.ice.identity");
			if (identity == null) {
				throw new RuntimeException(
						"Parameter 'fcm.ice.identity' not found");
			}
			fcm = new IceFCMClient(communicator, identity);
		}

		return fcm;
	}
}
