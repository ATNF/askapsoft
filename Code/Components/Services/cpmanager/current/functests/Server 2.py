import sys, traceback, Ice
from askap.ingest.ObsService import CPObsServiceImp

status = 0
ic = None
try:
    ic = Ice.initialize(sys.argv)
    adapter = ic.createObjectAdapterWithEndpoints("CPObsServiceAdapter", "default -p 10000")
    object = CPObsServiceImp()
    adapter.add(object, ic.stringToIdentity("CPObsServiceAdapter"))
    adapter.activate()
    ic.waitForShutdown()
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

