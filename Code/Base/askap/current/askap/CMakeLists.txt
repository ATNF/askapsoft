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
askap/askap/Application.h
askap/askap/AskapError.h
askap/askap/AskapLogging.h
askap/askap/AskapTypes.h
askap/askap/AskapUtil.h
askap/askap/CasaSyncHelper.h
askap/askap/CircularBuffer.h
askap/askap/ISignalHandler.h
askap/askap/IndexConverter.h
askap/askap/IndexedCompare.h
askap/askap/Log4cxxLogSink.h
askap/askap/MapKeyIterator.h
askap/askap/RangePartition.h
askap/askap/SignalCounter.h
askap/askap/SignalManagerSingleton.h
askap/askap/StatReporter.h
DESTINATION include/askap/askap
)

