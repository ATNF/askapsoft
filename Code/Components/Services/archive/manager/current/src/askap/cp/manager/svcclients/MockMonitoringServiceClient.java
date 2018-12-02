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

// ASKAPsoft imports

import askap.cp.manager.monitoring.MonitoringSingleton;
import askap.interfaces.monitoring.MonitorPoint;
import askap.interfaces.monitoring.PointStatus;
import askap.interfaces.schedblock.NoSuchSchedulingBlockException;
import askap.interfaces.schedblock.ParameterException;
import askap.util.ParameterSet;
import askap.util.TypedValueUtils;
import com.google.gson.*;
import com.google.gson.stream.JsonReader;

import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * This class provides a mock implementation of IDataServiceClient. It is
 * instantiated with the observation parameters for sbid == 0 and will return
 * the obsparameters when getObsParameters() is called.
 */
public class MockMonitoringServiceClient implements IMonitoringServiceClient {

    private final String ingestName;
    private List<MonitorPoint> pointList = new ArrayList<MonitorPoint>();

    MockMonitoringServiceClient(String ingestName, String fileName) {
        this.ingestName = ingestName;

        try {
            // read json object from given file
            Gson gson = new Gson();
            JsonReader reader = new JsonReader(new FileReader(fileName));

            JsonParser parser = new JsonParser();
            JsonObject jsonPoints = parser.parse(reader).getAsJsonObject();

            if (jsonPoints.get(ingestName) != null) {
                JsonArray points = jsonPoints.get(ingestName).getAsJsonArray();

                for (int i=0; i< points.size(); i++) {
                    JsonObject pointObj = points.get(i).getAsJsonObject();
                    MonitorPoint point = new MonitorPoint();
                    point.name = pointObj.get("name").getAsString();

                    JsonElement value = pointObj.get("value");
                    if (value.getAsJsonPrimitive().isNumber()) {
                        Number numValue = value.getAsJsonPrimitive().getAsNumber();

                        double dVal = numValue.doubleValue();
                        long lVal = numValue.longValue();

                        if (lVal == dVal)
                            point.value = TypedValueUtils.object2TypedValue(lVal);
                        else
                            point.value = TypedValueUtils.object2TypedValue(dVal);
                    } else
                        point.value = TypedValueUtils.object2TypedValue(value.getAsJsonPrimitive().getAsString());


                    point.status = PointStatus.valueOf(pointObj.get("status").getAsString());

                    if (pointObj.get("unit") != null)
                        point.unit = pointObj.get("unit").getAsString();

                    pointList.add(point);
                }
            }
        } catch (Exception e) {
            System.err.println("IOException reading " + fileName
                    + ": " + e.getMessage());
        }

    }


    @Override
    public List<MonitorPoint> get() {

        List<MonitorPoint> mpoints = new ArrayList<MonitorPoint>();
        long timestamp = MonitoringSingleton.getCurrentTimeMJD();

        for (MonitorPoint point : pointList) {
            MonitorPoint newPoint = (MonitorPoint) point.clone();
            point.timestamp = timestamp;
            mpoints.add(newPoint);
        }

        return mpoints;
    }

}
