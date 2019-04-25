#
# base/askapparallel
#
add_library(askapparallel
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


