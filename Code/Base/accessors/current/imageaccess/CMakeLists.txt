#
# base/accessors/imageaccess
#
add_library(imageaccess OBJECT
BeamLogger.cc
CasaImageAccess.cc
FITSImageRW.cc
FitsImageAccess.cc
IImageAccess.cc
ImageAccessFactory.cc
)

set_property(TARGET imageaccess PROPERTY POSITION_INDEPENDENT_CODE ON)

install (FILES
BeamLogger.h
CasaImageAccess.h
FITSImageRW.h
FitsImageAccess.h
IImageAccess.h
ImageAccessFactory.h

DESTINATION include/askap/imageaccess
)

