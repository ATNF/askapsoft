#!/usr/bin/bash

# General

useModuleExecs=true

if [ $useModuleExecs == true ]; then
    if [ "`module list -t 2>&1 | grep askapsoft`" == "" ]; then
        module load askapsoft
    fi
    slicer=makeModelSlice
    createFITS=createFITS
    rndgains=randomgains
    csim=csimulator
    ccal=ccalibrator
    cim=cimager
    mssplit=mssplit
    msmerge=msmerge
else
    slicer=${ASKAP_ROOT}/Code/Components/Analysis/simulations/current/apps/makeModelSlice.sh
    createFITS=${ASKAP_ROOT}/Code/Components/Analysis/simulations/current/apps/createFITS.sh
    rndgains=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/randomgains.sh
    csim=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/csimulator.sh
    ccal=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/ccalibrator.sh
    cim=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/cimager.sh
    mssplit=${ASKAP_ROOT}/Code/Components/CP/pipelinetasks/current/apps/mssplit.sh
    msmerge=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/msmerge.sh
fi

askapconfig=${ASKAP_ROOT}/Code/Components/Synthesis/testdata/current/simulation/stdtest/definitions

now=`date +%F-%H%M`

parsetdir=parsets
logdir=logs
slurms=slurmFiles
slurmOutput=slurmOutput
msdir=MS
chunkdir=../ModelImages/Chunks
slicedir=../ModelImages/Slices

doCreateModel=false
# Whether to slice up the model prior to simulating - set to false
# if we've already done this
doSlice=false

doCalibrator=false
doScience=true

doCorrupt=false
randomgainsparset=${parsetdir}/randomgains.in

doNoise=true
Tsys=50

nfeeds=36
#nfeeds=1
antennaConfig=ADE12
nant=12
#antennaConfig=ADE6
#nant=6

pol="XX XY YX YY"

inttime=5s

##################################
# For 1934 calibration observation
#

firstPointingID=0
lastPointingID=`expr ${nfeeds} - 1`

msbaseCal=calibrator_J1934m638_${inttime}_${now}

ra1934=294.854275
ra1934str=19h39m25.036
dec1934=-63.712675
dec1934str=-63.42.45.63
direction1934="[${ra1934str}, ${dec1934str}, J2000]"

. ${simScripts}/makeCalHArange.sh

#Gridding parameters
nw=201
os=8


###############################
# For science field observation
#

msbaseSci=sciencefield_${antennaConfig}_SKADS_${inttime}_${nfeeds}beam_${now}
if [ $nfeeds -eq 36 ]; then
    feeds=${askapconfig}/ASKAP${nfeeds}feeds-footprint-square_6x6.in
else
    feeds=${askapconfig}/ASKAP${nfeeds}feeds.in
fi
antennaParset=${askapconfig}/${antennaConfig}.in

# Time required for the csimulator jobs
TIME_CSIM_JOB=2:30:00

# Set this to true if we want to mimic the correlator layout (48MHz blocks, each producing its own MS)
matchCorrelatorLayout=true
if [ $matchCorrelatorLayout == true ]; then
# Number of MSs to end up with after 2nd stage of merging
    NUM_FINAL_MS=7
    msbaseSci=sciencefield_${antennaConfig}_by${NUM_FINAL_MS}_SKADS_${inttime}_${nfeeds}beam_${now}
    NGROUPS_CSIM=28
    NWORKERS_CSIM=216
    chanPerMSchunk=3
    NPPN_CSIM=3
    TILENCHAN=54
else
# Number of MSs to end up with after 2nd stage of merging
    NUM_FINAL_MS=8
    NGROUPS_CSIM=32
    NWORKERS_CSIM=171
    chanPerMSchunk=3
    NPPN_CSIM=3
    TILENCHAN=54
fi

NCPU_CSIM=`echo $NWORKERS_CSIM | awk '{print $1+1}'`
if [ `echo $NGROUPS_CSIM $NUM_FINAL_MS | awk '{print $1 % $2 }'` -ne 0 ]; then
    echo "Number of groups (${NGROUPS_CSIM}) not a multiple of number of final MSs (${NUM_FINAL_MS})."
    echo "Not running."
    doSubmit=false
else
    NUM_GROUPS_PER_FINAL=`echo $NGROUPS_CSIM $NUM_FINAL_MS | awk '{print $1/$2}'`
fi

# Whether to remove the intermediate MSs once the merging step has
# completed successfully
CLOBBER_INTERMEDIATE_MS=true

catdir=/group/astronomy856/whi550/Simulations/InputCatalogue
sourcelist=master_possum_catalogue_trim10x10deg.dat

doFlatSpectrum=false
baseimage=SKADS_model_matchADE
writeByNode=true
createTT_CR=true

npixModel=4096
nsubxCR=9
nsubyCR=11
CREATORTASKS=`echo $nsubxCR $nsubyCR | awk '{print $1*$2+1}'`
CREATORWORKERPERNODE=1
CREATORNODES=`echo $CREATORTASKS ${CREATORWORKERPERNODE} | awk '{print int($1/$2)}'`
SLICERWIDTH=100
SLICERNPPN=20

#databaseCR=POSSUM
databaseCR=POSSUMHI
#databaseCR=joint

posType=deg
PAunits=rad
useGaussianComponents=true
if [ $databaseCR == "POSSUM" ]; then
    listtypeCR=continuum
    baseimage="${baseimage}_cont"
elif [ $databaseCR == "POSSUMHI" ]; then
    #listtypeCR=spectralline
    listtypeCR=continuum
    baseimage="${baseimage}_C+HI"
elif [ $databaseCR == "joint" ]; then
    # This is the case for when we've combined the continuum and HI
    # models into "_joint" models
    baseimage="${baseimage}_joint"
fi
modelimage=${chunkdir}/${baseimage}

# Size and pixel scale of spatial axes
npix=4096
rpix=`echo $npix | awk '{print $1/2}'`
cellsize=6
delt=`echo $cellsize | awk '{print $1/3600.}'`

# Central position for the input model
ra=187.5
dec=-45.0
# And how that is translated for the csimulator jobs
raStringVis="12h30m00.000"
decStringVis="-45.00.00"
baseDirection="[${raStringVis}, ${decStringVis}, J2000]"

# Does the catalogue need precessing? If so, central position of catalogue.
WCSsources=true
raCat=0.
decCat=0.

# Spectral axis - full spectral range & resolution
freqChanZeroMHz=1421
#nchan=16416
nchan=18144
rchan=0
chanw=-18.5185185e3
rfreq=`echo ${freqChanZeroMHz} | awk '{printf "%8.6e",$1*1.e6}'`
basefreq=`echo $nchan $rchan $rfreq $chanw | awk '{printf "%8.6e",$3 + $4*($2+$1/2)}'`

# Polarisation axis - use full stokes for these models
nstokes=4
rstokes=0
stokesZero=0
dstokes=1


spwbaseSci=${parsetdir}/spws_sciencefield

observationLengthHours=8
# duration for csimulator parset - have observation evenly split over
# transit, so give hour angle start/stop times
dur=`echo $observationLengthHours | awk '{print $1/2.}'`
