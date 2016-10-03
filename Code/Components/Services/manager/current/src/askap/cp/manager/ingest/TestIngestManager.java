/**
 *  Copyright (c) 2014-2015 CSIRO - Australia Telescope National Facility (ATNF)
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
package askap.cp.manager.ingest;

import askap.cp.manager.svcclients.FuncTestReporterClient;
import java.io.File;

import org.apache.log4j.Logger;

import askap.util.ParameterSet;

/**
 * This test ingest pipeline manager does nothing when execute/abort
 * is called except log messages at level INFO.
 */
public class TestIngestManager extends AbstractIngestManager {

    /**
     * Logger
     */
    private static final Logger logger = Logger.getLogger(TestIngestManager.class.getName());
	private final FuncTestReporterClient funcTestReporterClient;

    /**
     * Constructor
	 * @param parset
	 * @param client
     */
    public TestIngestManager(ParameterSet parset, FuncTestReporterClient client) {
        super(parset);
		funcTestReporterClient = client;
    }

	/**
	 * 
	 * @param timeout
	 * @return 
	 */
	@Override
    public boolean waitIngest(long timeout) {
        logger.info("waitIngest");
		funcTestReporterClient.methodCalled("waitIngest");
		return super.waitIngest(timeout);
	}

    /**
     * Dummy execute - does nothing except log a message
	 * @param workdir
     */
    @Override
    protected void executeIngestPipeline(File workdir) {
        logger.info("Execute");
		funcTestReporterClient.methodCalled("startIngest");
    }

    /**
     * Dummy abort - does nothing except log a message
     */
    @Override
    protected void abortIngestPipeline() {
        logger.info("Abort");
		funcTestReporterClient.methodCalled("abortIngest");
    }

    /**
     * Always returns false. This essentially mimics an ingest pipeline
     * that starts and finishes immediately.
	 * @return false
     */
    @Override
    public boolean isRunning() {
        return false;
    }
}
