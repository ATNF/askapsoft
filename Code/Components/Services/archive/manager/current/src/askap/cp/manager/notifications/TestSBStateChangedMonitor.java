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
package askap.cp.manager.notifications;

import askap.cp.manager.svcclients.FuncTestReporterClient;
import askap.interfaces.schedblock.ObsState;
import org.apache.log4j.Logger;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public final class TestSBStateChangedMonitor extends SBStateMonitor {

	private static final Logger logger = Logger.getLogger(TestSBStateChangedMonitor.class.getName());
	private final FuncTestReporterClient funcTestReporterClient;

	/**
	 * Instantiate the TestSBStateChangedMonitor
	 */
	public TestSBStateChangedMonitor(FuncTestReporterClient client) { 
		funcTestReporterClient = client;
	}

	@Override
	public void notify(long sbid, ObsState newState, String updateTime) {
		logger.debug("Notifying SB state change, new state: " + newState);
		funcTestReporterClient.notifySBStateChanged(sbid, newState);
	}
	
}
