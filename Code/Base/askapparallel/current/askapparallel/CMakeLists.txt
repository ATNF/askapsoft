#
# base/askapparallel
#
add_library(askap_askapparallel
AskapParallel.cc
BlobIBufMW.cc
BlobOBufMW.cc
MPIComms.cc
)

install (FILES
AskapParallel.h
BlobIBufMW.h
BlobOBufMW.h
MPIComms.h
DESTINATION include/askap/askapparallel
)

target_link_libraries (
	askap_askapparallel
	${CASACORE_LIBRARIES}
	${log4cxx_LIBRARY}
	${ASKAP_LIBRARY}
	${LofarBlob_LIBRARY}
)

