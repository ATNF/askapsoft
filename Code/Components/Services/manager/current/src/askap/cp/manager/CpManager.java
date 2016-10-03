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

import askap.cp.manager.notifications.SBStateMonitor;
import IceStorm.NoSuchTopic;
import IceStorm.TopicPrx;
import org.apache.log4j.Logger;

import askap.cp.manager.monitoring.MonitoringSingleton;
import askap.cp.manager.notifications.SBStateMonitorFactory;
import askap.util.ServiceApplication;
import askap.util.ServiceManager;

public final class CpManager extends ServiceApplication {

	/**
	 * Logger
	 */
	private static final Logger logger = Logger.getLogger(CpManager.class.getName());

	/**	
	 * The IceStorm SBStateChanged topic
	 */
	IceStorm.TopicPrx sbStateChangedTopic = null;

	/**
	 * The IceStorm SBStateChanged subscriber
	 */
	Ice.ObjectPrx sbStateChangedSubscriber = null;

	/**
	 * @see askap.cp.manager.ServiceApplication#run(java.lang.String[])
	 */
	@Override
	public int run(String[] args) {
		try {
			logger.info("ASKAP Central Processor Manager");

			final String serviceName = config().getString("ice.servicename");
			if (serviceName == null) {
				logger.error("Parameter 'ice.servicename' not found");
				return 1;
			}
			final String adapterName = config().getString("ice.adaptername");
			if (adapterName == null) {
				logger.error("Parameter 'ice.adaptername' not found");
				return 1;
			}

			// Create and register the ObsService object
			ObsService svc = ObsService.Create(communicator(), config());

			// Initialise monitoring interface if configured
			boolean monitoring = config().getBoolean("monitoring.enabled", false);
			if (monitoring) {
				boolean status = initMonitoring();
				if (!status) {
					logger.error("Monitoring sub-system failed to initialise correctly");
				}
			}

			// Initialise the Scheduling block state change subscriber
			boolean monitorSbStates = config().getBoolean("sbstatemonitor.enabled", false);
			if (monitorSbStates) {
				// TODO: pass in config if required.
				if (!initSBStateMonitor()) {
					logger.error("Scheduling block state change subscriber failed to initialise");
				}
			}

			// Create the service manager for Ice orchestration
			// We don't use the utility ServiceManager.run static method anymore,
			// as IceStorm subscriptions must be unsubscribed after waitForShutdown()
			// and before stop().
			ServiceManager manager = new ServiceManager();

			// Kick it into life
			manager.start(communicator(), svc, serviceName, adapterName);

			// Block until shutdown
			manager.waitForShutdown();

			// Unsubscribe all IceStorm subscribers
			if (monitorSbStates) {
				sbStateChangedTopic.unsubscribe(sbStateChangedSubscriber);
			}

			// And now we are done
			manager.stop();

			if (monitoring) {
				MonitoringSingleton.destroy();
			}
		} catch (Exception e) {
			logger.error("Unexpected exception: " + e);
		}

		return 0;
	}

	/**
	 * Main
	 *
	 * @param args command line arguments
	 */
	public static void main(String[] args) {
		CpManager svr = new CpManager();
		int status = svr.servicemain(args);
		System.exit(status);
	}

	/**
	 * Initialise the monitoring singleton.
	 *
	 * @return true if the monitoring sub-system was correctly initialised,
	 * otherwise false.
	 */
	private boolean initMonitoring() {
		final String key1 = "monitoring.ice.servicename";
		final String key2 = "monitoring.ice.adaptername";
		String serviceName = config().getString(key1);
		if (serviceName == null) {
			logger.error("Parameter '" + key1 + "' not found");
			return false;
		}
		String adapterName = config().getString(key2);
		if (adapterName == null) {
			logger.error("Parameter '" + key2 + "' not found");
			return false;
		}

		if (logger.isDebugEnabled()) {
			logger.debug("monitoring.ice.servicename: " + serviceName);
			logger.debug("monitoring.ice.adaptername: " + adapterName);
		}

		MonitoringSingleton.init(communicator(), serviceName, adapterName);
		return true;
	}

	/**
	 * Initialises the Scheduling block state change monitor.
	 *
	 * @return true if the state change monitor was correctly initialised;
	 * otherwise false.
	 * 
	 * @throws RuntimeException
	 */
	private boolean initSBStateMonitor() {
		logger.debug("initialising SB state change subscriber");

		// We need to sleep for a little while, so the IceStorm process can 
		// start up during functional tests.
		try {
			Thread.sleep(1000);
		}
		catch (InterruptedException ex) {
			logger.debug("Sleep interrupted");
		}

		// Get the names of the topic manager and topic from the parset
		String topicManagerName = config().getString("sbstatemonitor.topicmanager");
		if (topicManagerName == null) {
			logger.error("Parameter 'sbstatemonitor.topicmanager' not found");
			return false;
		}

		String topicName = config().getString("sbstatemonitor.topic");
		if (topicName == null) {
			logger.error("Parameter 'sbstatemonitor.topic' not found");
			return false;
		}

		// get a proxy to the IceStorm topic manager
		logger.debug("Getting topic manager proxy: " + topicManagerName);
		Ice.ObjectPrx obj = communicator().stringToProxy(topicManagerName);
		IceStorm.TopicManagerPrx topicManager = IceStorm.TopicManagerPrxHelper.checkedCast(obj);
        if (topicManager == null) {
            throw new RuntimeException("Failed to get IceStorm topic manager");
        }

		// Create the object adapter to bind our subscriber servant to the Ice runtime
		logger.debug("Getting Ice object adapter: SBStateMonitorAdapter");
		Ice.ObjectAdapter adapter = communicator().createObjectAdapterWithEndpoints(
				"SBStateMonitorAdapter",
				"tcp");
        if (adapter == null) {
            throw new RuntimeException("ICE adapter initialisation failed");
        }

		// Instantiate our schedblock state monitor servant and register with the adapter
		SBStateMonitor monitor = SBStateMonitorFactory.getInstance(
			config().getString("sbstatemonitor.notificationagenttype"),
			config(),
			communicator());
		sbStateChangedSubscriber = adapter.addWithUUID(monitor).ice_oneway();
		adapter.activate();

		// subscribe to the topic
		try {
			sbStateChangedTopic = getTopic(topicManager, topicName);
			java.util.Map<String, String> qos = null;
			sbStateChangedTopic.subscribeAndGetPublisher(qos, sbStateChangedSubscriber);
		}
		catch (IceStorm.NoSuchTopic | IceStorm.AlreadySubscribed | IceStorm.BadQoS ex) {
            throw new RuntimeException("ICE topic subscription failed", ex);
		}

		return true;
	}

	/**
	 * Gets an IceStorm topic, creating the topic if it does not already exist.
	 * @param topicManager: The IceStorm topic manager
	 * @param topicName: the topic name
	 * @return The IceStorm TopicPrx instance.
	 * 
	 * @throws IceStorm.NoSuchTopic
	 */
	private static TopicPrx getTopic(
			IceStorm.TopicManagerPrx topicManager,
			String topicName) throws NoSuchTopic {
		IceStorm.TopicPrx topic = null;
		try {
			topic = topicManager.retrieve(topicName);
		}
		catch (IceStorm.NoSuchTopic nst) {
			try {
				topic = topicManager.create(topicName);
			}
			catch (IceStorm.TopicExists te) {
				topic = topicManager.retrieve(topicName);
			}
		}

		return topic;
	}
}