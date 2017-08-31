import sys, traceback, Ice, time

from askap.ingest import get_service

from askap.slice import CP
from askap.slice import MonitoringProvider

# noinspection PyUnresolvedReferences
from askap.interfaces.cp import ICPObsServicePrx

# noinspection PyUnresolvedReferences
from askap.interfaces.monitoring import MonitoringProviderPrx


status = 0
ic = None
try:
    ic = Ice.initialize(sys.argv)
#    base = ic.stringToProxy("CPObsServiceAdapter:default -p 10000 -h localhost")

#    base = ic.stringToProxy("MonitoringService@IngestPipelineMonitoringAdapter0")

#    MonitoringProvider = MonitoringProviderPrx.checkedCast(base)

#    provider = get_service("MonitoringService@IngestPipelineMonitoringAdapter36",
#                           MonitoringProviderPrx, ic)

#    CPObsService = ICPObsServicePrx.checkedCast(base)
#    if not CPObsService:
#        raise RuntimeError("Invalid proxy")

#    provider = get_service("MonitoringService@IngestManagerMonitoringAdapter",
#                       MonitoringProviderPrx, ic)

    CPObsService = cpservice = get_service(
                "CentralProcessorService@IngestManagerAdapter",
                ICPObsServicePrx,
                ic)

    CPObsService.abortObs()

    CPObsService.startObs(5)

    time.sleep(5)

    monitoringservice = get_service("MonitoringService@IngestManagerMonitoringAdapter",
                                         MonitoringProviderPrx,
                                         ic)


    point_names = ['ingest.running', 'ingest0.cp.ingest.obs.StartFreq', 'ingest0.cp.ingest.obs.nChan',
                   'ingest0.cp.ingest.obs.ChanWidth', 'ingest36.cp.ingest.obs.FieldName',
                   'ingest36.cp.ingest.obs.ScanId']

    points = monitoringservice.get(point_names)

    time.sleep(5)

    CPObsService.abortObs()
    points = monitoringservice.get(point_names)


#    point_names = ["cp.ingest.obs.StartFreq"]
#    points = provider.get(point_names)

#    print "Got points ", points

except:
    traceback.print_exc()
    status = 1
 
if ic:
    # Clean up
    try:
        ic.destroy()
    except:
        traceback.print_exc()
        status = 1
 
sys.exit(status)
