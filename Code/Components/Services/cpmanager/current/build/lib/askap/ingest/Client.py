import sys, traceback, Ice

from askap.slice import CP
from askap.interfaces.cp import ICPObsServicePrx

status = 0
ic = None
try:
    ic = Ice.initialize(sys.argv)
    base = ic.stringToProxy("CPObsServiceAdapter:default -p 10000")
    CPObsService = ICPObsServicePrx.checkedCast(base)
    if not CPObsService:
        raise RuntimeError("Invalid proxy")

    CPObsService.abortObs()

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
