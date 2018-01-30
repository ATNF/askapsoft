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

import java.util.*;

/**
 *
 * @author Daniel Collins <daniel.collins@csiro.au>
 */
public final class ClientServiceFactory {

	private static final Logger logger = Logger.getLogger(ClientServiceFactory.class.getName());

	private IFCMClient fcmClient = null;
	private IDataServiceClient dataServiceClient = null;

	private Map<String, IMonitoringServiceClient> ingestMonitoringServices = new HashMap<String, IMonitoringServiceClient>();
	private static ClientServiceFactory clientFactory = null;

	private ParameterSet config;
	private Communicator communicator;

	private ClientServiceFactory(ParameterSet config, Communicator communicator) {
		this.communicator = communicator;
		this.config = config;

		createDataServerInstance();
		createFCMInstance();
        createIngestMonitoringServices();
	}

	public static synchronized void init(ParameterSet config, Communicator communicator) {
		if (clientFactory==null) {
			clientFactory = new ClientServiceFactory(config, communicator);
		}
	}

	/**
	 * Factory method for FCM clients
	 *
	 * @return IFCMClient
	 */
	private IFCMClient createFCMInstance() {

		if (fcmClient != null)
			return fcmClient;

		// Instantiate real or mock FCM
		boolean mockfcm = config.getBoolean("fcm.mock", false);
		if (mockfcm) {
			logger.debug("ObsService factory: creating mock FCM");
			fcmClient = new MockFCMClient(config.getString("fcm.mock.filename"));
		} else {
			logger.debug("ObsService factory: creating FCM");
			String identity = config.getString("fcm.ice.identity");
			if (identity == null) {
				throw new RuntimeException(
						"Parameter 'fcm.ice.identity' not found");
			}
			fcmClient = new IceFCMClient(communicator, identity);
		}

		return fcmClient;
	}

	public static IFCMClient getFCMInstance() {
		return clientFactory.fcmClient;
	}

	/**
	 * Factory method for DataService clients
	 *
	 * @return IDataServiceClient
	 */
	private IDataServiceClient createDataServerInstance() {

		if (dataServiceClient != null)
			return dataServiceClient;

		// Instantiate real or mock FCM
		boolean mockdataservice = config.getBoolean("dataservice.mock", false);
		if (mockdataservice) {
			logger.debug("ObsService factory: creating mock DataService");
			String mockfile = config.getString("dataservice.mock.filename", "");
			if (mockfile==null)
				throw new RuntimeException(
						"Parameter 'dataservice.mock.filename' not found");

			dataServiceClient = new MockDataServiceClient(mockfile);
		} else {
			logger.debug("ObsService factory: creating dataservice");
			String identity = config.getString("dataservice.ice.identity");
			if (identity == null) {
				throw new RuntimeException(
						"Parameter 'dataservice.ice.identity' not found");
			}
			dataServiceClient = new IceDataServiceClient(communicator, identity);
		}

		return dataServiceClient;
	}

	public static IDataServiceClient getDataServiceClient() {
		return clientFactory.dataServiceClient;
	}

	private void createIngestMonitoringServices() {
		if (!ingestMonitoringServices.isEmpty())
			return;

		Object vectorValue[] = config.getVectorValue("ingest.monitoring.points", java.lang.String.class.getName());
		String[] pointnames = Arrays.copyOf(vectorValue, vectorValue.length, String[].class);

		// Instantiate real or mock Ingest Monitoring Services
		boolean mockmonitorservice = config.getBoolean("monitorservice.mock", false);

		logger.debug("ObsService factory: creating monitoringServices");
		String adaptorBase = config.getString("ingest.monitoring.adaptor.base",
				"MonitoringService@IngestPipelineMonitoringAdapter");
		int rankCount = config.getInteger("ingest.monitoring.rank.count", 61);


		if (vectorValue==null || vectorValue.length<=0)
			throw new RuntimeException(
					"Parameter 'ingest.monitoring.points' not found");

		for (int i=0; i<rankCount; i++) {
			String ingest = "ingest" + i;
			String adaptorName = adaptorBase + i;

			IMonitoringServiceClient monitoringClient = null;

			if (mockmonitorservice)
				monitoringClient = new MockMonitoringServiceClient(ingest, config.getString("monitorservice.mock.filename"));
			else
				monitoringClient = new IceMonitoringServiceClient(communicator, adaptorName, pointnames);

			ingestMonitoringServices.put(ingest, monitoringClient);
		}

		return;
	}

	public static Map<String, IMonitoringServiceClient> getIngestServices() {
		return clientFactory.ingestMonitoringServices;
	}
}
