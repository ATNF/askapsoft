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

target_link_libraries(
	askap_askap
	${CASACORE_LIBRARIES}
	${log4cxx_LIBRARY}
	${Boost_LIBRARIES}
	${LofarCommon_LIBRARY}
	)

install (
	TARGETS askap_askap
	RUNTIME DESTINATION bin
	LIBRARY DESTINATION lib
	ARCHIVE DESTINATION lib
	LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
)
