package askap.cp.manager.svcclients;


import Ice.Communicator;
import Ice.Current;
import Ice.ObjectAdapter;
import askap.interfaces.cp._ICPFuncTestReporterDisp;
import askap.interfaces.schedblock.ObsState;

/**
 * Created by wu049 on 12/5/17.
 */
public class ICPFuncTestReporterStub extends _ICPFuncTestReporterDisp {

    @Override
    public void sbStateChangedNotification(long l, ObsState obsState, Current current) {

    }

    @Override
    public void methodCalled(String s, Current current) {
        System.out.println("method called: " + s);
    }

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

            ObjectAdapter adapter = ic.createObjectAdapter("FuncTestServerAdapter");

            Ice.Object object = new ICPFuncTestReporterStub();
            adapter.add( object, ic.stringToIdentity("FuncTestReporter"));

            adapter.activate();

            ic.waitForShutdown();

        } catch (Exception e) {
            e.printStackTrace(System.err);
        }


        System.exit(0);
    }
}

