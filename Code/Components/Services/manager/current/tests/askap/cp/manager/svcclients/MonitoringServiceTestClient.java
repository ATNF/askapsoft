package askap.cp.manager.svcclients;


import Ice.Communicator;
import Ice.Current;
import Ice.ObjectAdapter;
import askap.interfaces.cp._ICPFuncTestReporterDisp;
import askap.interfaces.monitoring.MonitorPoint;
import askap.interfaces.monitoring.MonitoringProviderPrx;
import askap.interfaces.monitoring.MonitoringProviderPrxHelper;
import askap.interfaces.schedblock.ObsState;

/**
 * Created by wu049 on 12/5/17.
 *
 * icegridregistry --Ice.Config=config-files/ice.cfg
 *
 */
public class MonitoringServiceTestClient {

    public static void main(String[] args) {
        Communicator ic = null;

        try {

            Ice.InitializationData id = new Ice.InitializationData();

            Ice.Properties props = Ice.Util.createProperties();
            props.setProperty("Ice.Default.Locator",
//                    "IceGrid/Locator:tcp -h localhost -p 4061");
                "IceGrid/Locator:tcp -h 130.155.194.244 -p 4061");

            props.setProperty("Ice.MessageSizeMax", "131072");

            props.setProperty("Ice.Trace.Network", "0");
            props.setProperty("Ice.Trace.Protocol", "0");
            props.setProperty("Ice.Trace.Locator", "0");
            props.setProperty("Ice.IPv6", "0");

            props.setProperty("FuncTestServerAdapter.AdapterId", "FuncTestServerAdapter");
            props.setProperty("FuncTestServerAdapter.Endpoints", "tcp");

            id.properties = props;
            ic = Ice.Util.initialize(id);

            Ice.ObjectPrx obj = ic.stringToProxy("MonitoringService@CentralProcessorMonitoringAdapter");
            MonitoringProviderPrx itsProxy = MonitoringProviderPrxHelper.checkedCast(obj);

            MonitorPoint[] points = itsProxy.get(new String[]{"ingest36.cp.ingest.obs.FieldName",
                                                                "ingest36.cp.ingest.obs.ScanId"});
            System.out.println("Got " + points.length + " points for ingest36");

            points = itsProxy.get(new String[]{"ingest0.cp.ingest.obs.StartFreq",
                                                "ingest0.cp.ingest.obs.nChan",
                                                "ingest0.cp.ingest.obs.ChanWidth"});
            System.out.println("Got " + points.length + " points for ingest0");


        } catch (Exception e) {
            e.printStackTrace(System.err);
        }

        System.exit(0);
    }
}

