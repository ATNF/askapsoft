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

import askap.cp.manager.monitoring.MonitorPointStatus;
import askap.cp.manager.monitoring.MonitoringSingleton;
import askap.cp.manager.svcclients.ClientServiceFactory;
import askap.cp.manager.svcclients.IMonitoringServiceClient;
import askap.interfaces.TypedValueDouble;
import askap.interfaces.TypedValueLong;
import askap.interfaces.monitoring.MonitorPoint;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonObject;
import org.apache.log4j.Logger;

import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * This class implement a thread for monitoring the ingest pipeline. The ingest
 * monitor thread is responsible for keeping the cp related monitoring
 * point data current.
 *
 * @author Ben Humphreys, Xinyu Wu
 */
public class IngestMonitorThread implements Runnable {
    private final AbstractIngestManager ingestManager;
    private static final Logger logger = Logger.getLogger(IngestMonitorThread.class.getName());

    private static Gson theirGson = new GsonBuilder().setPrettyPrinting().create();

    /*
     * remember the last successfully updated SB, so we don't need to keep updating
     * Dataservice with the same info.
     */
    private long lastSBID = -10;

    /**
     * Constructor
     *
     * @param manager a AbstractIngestManager instance that this class
     *                calls to get the status of the ingest pipeline.
     * @throws NullPointerException if the "manager" parameter is null
     */
    public IngestMonitorThread(AbstractIngestManager manager) {
        if (manager == null) {
            throw new NullPointerException("Manager reference must be non-null");
        }
        ingestManager = manager;
    }

    /**
     * Entry point
     */
    @Override
    public void run() {
        while (!Thread.currentThread().isInterrupted()) {
            MonitoringSingleton mon = MonitoringSingleton.getInstance();
            if (mon == null) return;

            long sbid = ingestManager.isRunning();
            mon.update("ingest.running", sbid>=0, MonitorPointStatus.OK);

            // if ingestManager is running poll monitoring points and publish them
            if (sbid>=0)
                processIngestMonitoringPoints(sbid);

            try {
                Thread.sleep(1000);
            } catch (InterruptedException e) {
                // Polling more quickly is fine in the event
                // the sleep is interrupted
            }
        }
    }

    /**
     * This method polls all the ingest processes for the latest values and publish them on
     * a single channel with ingest name add to the beginning of the point name.
     *
     * It also extracts info about the observation, such as start frequency, number of channels,
     * bandwidth and centre frequency and writes them to DataService.
     *
     */
    private void processIngestMonitoringPoints(long sbid) {

        MonitoringSingleton mon = MonitoringSingleton.getInstance();
        Map<String, IMonitoringServiceClient> ingestMonitoringClients = ClientServiceFactory.getIngestServices();

        for (String ingestName : ingestMonitoringClients.keySet()) {
            IMonitoringServiceClient client = ingestMonitoringClients.get(ingestName);
            List<MonitorPoint> pointValues = client.get();

            if (pointValues != null && !pointValues.isEmpty()) {

                updateDataService(sbid, pointValues);

                for (MonitorPoint point : pointValues) {
                    point.name = ingestName + "." + point.name;
                    mon.update(point);
                }
            }
        }
    }

    private void updateDataService(long sbid, List<MonitorPoint> pointValues) {

        // don't need to update Dataservice if we have already. Assume the obs detail
        // does not change during observation.
        if (this.lastSBID == sbid)
            return;

        MonitorPoint startFreqPoint = null;
        MonitorPoint nChanPoint = null;
        MonitorPoint chanWidthPoint = null;

        for (MonitorPoint point : pointValues) {

            if (point.name.equals("cp.ingest.obs.StartFreq"))
                startFreqPoint = point;
            else if (point.name.equals("cp.ingest.obs.nChan"))
                nChanPoint = point;
            else if (point.name.equals("cp.ingest.obs.ChanWidth"))
                chanWidthPoint = point;
        }

        /*
         * write observation details as a json string:
         *
         * {
         *     "nChan":  {"value": 10368},
         *     "ChanWidth":  {"value": 1344.49, "unit": "MHz"},
         *     "StartFreq":  {"value": 1344.49, "unit": "MHz"},
         *     "CentreFreq":  {"value": 2344.49, "unit": "MHz"},
         *     "BandWidth":  {"value": 192.00, "unit": "MHz"}
         *  }
         *
         */
        if (startFreqPoint != null && nChanPoint != null && chanWidthPoint != null) {

            double startFreq = ((TypedValueDouble) startFreqPoint.value).value;
            double chanWidth = ((TypedValueDouble) chanWidthPoint.value).value;
            long nChan = ((TypedValueLong) nChanPoint.value).value;

            double bandWidth = 0;
            if (chanWidthPoint.unit.equalsIgnoreCase("khz"))
                bandWidth = chanWidth * nChan / 1000;
            else // assume mhz
                bandWidth = chanWidth * nChan;

            double centreFreq = startFreq + bandWidth / 2;

            JsonObject obsInfo = new JsonObject();

            JsonObject obj = new JsonObject();
            obj.addProperty("value", nChan);
            obsInfo.add("nChan", obj);

            obj = new JsonObject();
            obj.addProperty("value", chanWidth);
            obj.addProperty("unit", chanWidthPoint.unit);
            obsInfo.add("ChanWidth", obj);

            obj = new JsonObject();
            obj.addProperty("value", startFreq);
            obj.addProperty("unit", startFreqPoint.unit);
            obsInfo.add("StartFreq", obj);

            obj = new JsonObject();
            obj.addProperty("value", centreFreq);
            obj.addProperty("unit", startFreqPoint.unit);
            obsInfo.add("CentreFreq", obj);

            obj = new JsonObject();
            obj.addProperty("value", bandWidth);
            obj.addProperty("unit", startFreqPoint.unit);
            obsInfo.add("BandWidth", obj);


            Map<String, String> obsVar = new HashMap<String, String>();
            obsVar.put("obs.info", theirGson.toJson(obsInfo));

            try {
                ClientServiceFactory.getDataServiceClient().setObsVariables(sbid, obsVar);
                this.lastSBID = sbid;
            } catch (Exception e) {
                logger.error("Could not update Dataservice with observation details", e);
            }
        }
    }

}
