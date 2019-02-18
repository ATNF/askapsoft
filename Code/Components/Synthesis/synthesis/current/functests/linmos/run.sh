#!/bin/bash

OUTPUT=output.txt

export AIPSPATH=${ASKAP_ROOT}/Code/Base/accessors/current

if [ ! -x ../../apps/imager.sh ]; then
    echo imager.sh does not exit
fi

#
IMAGE=image.cont.fits
RESTORED=image.cont.restored.fits
PSF=psf.cont.fits
RESIDUAL=residual.cont.fits
WEIGHTS=weights.cont.fits

echo -n "Removing image cubes..."
rm -f *.fits

echo Done

echo -n Extracting measurement set...
tar -xvf ../linmos_1_chan.ms.tar.gz
echo Done

mpirun --oversubscribe -np 2 ../../apps/imager.sh -c msmfs.in | tee $OUTPUT
if [ $? -ne 0 ]; then
    echo Error: mpirun returned an error
    exit 1
fi

mpirun --oversubscribe -np 2 ../../apps/linmos-mpi.sh -c linmos.in | tee $OUTPUT
if [ $? -ne 0 ]; then
    echo Error: mpirun returned an error
    exit 1
fi


echo -n Removing measurement set...
rm -rf linmos_1_chan.ms
echo Done

# Check for instances of "Askap error"
grep -c "Askap error" $OUTPUT > /dev/null
if [ $? -ne 1 ]; then
    echo "Askap error reported in output.txt"
    exit 1
fi

# Check for instances of "Unexpected exception"
grep -c "Unexpected exception" $OUTPUT > /dev/null
if [ $? -ne 1 ]; then
    echo "Exception reported in output.txt"
    exit 1
fi
grep -c "BAD TERMINATION" $OUTPUT > /dev/null
if [ $? -ne 1 ]; then
    echo "MPI spotted bad termination (SEGV?)"
    exit 1
fi
# Check for the existance of the various image cubes
if [ ! -f ${IMAGE} ]; then
    echo "Error ${IMAGE} not created"
    exit 1
fi

if [ ! -f ${PSF}${IDX} ]; then
    echo "Error ${PSF} not created"
    exit 1
fi

if [ ! -f ${WEIGHTS}${IDX} ]; then
    echo "Error ${WEIGHTS} not created"
    exit 1
fi

echo Done

