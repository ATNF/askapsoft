#
# askap askap
#
add_library(askap_askap 
Application.cc
AskapError.cc
AskapUtil.cc
ISignalHandler.cc
IndexConverter.cc
Log4cxxLogSink.cc
RangePartition.cc
SignalCounter.cc
SignalManagerSingleton.cc
StatReporter.cc
)

install (FILES
Application.h
AskapError.h
AskapLogging.h
AskapTypes.h
AskapUtil.h
CasaSyncHelper.h
CircularBuffer.h
ISignalHandler.h
IndexConverter.h
IndexedCompare.h
Log4cxxLogSink.h
MapKeyIterator.h
RangePartition.h
SignalCounter.h
SignalManagerSingleton.h
StatReporter.h
DESTINATION include/askap/askap
)

