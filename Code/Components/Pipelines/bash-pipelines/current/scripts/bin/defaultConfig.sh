#!/bin/bash -l
#
# The complete set of user-defined parameters, along with their
# default values. All parameters defined herein can be set in the
# input file, and any value given there will override the default
# value set here.
#
# @copyright (c) 2015 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Matthew Whiting <Matthew.Whiting@csiro.au>
#

####################
# Whether to submit the scripts to the queue.
SUBMIT_JOBS=false

####################
# Cluster to submit to
CLUSTER=galaxy
# Queue to submit to
QUEUE=workq
# Reservation to use. If no reservation available, leave as blank
RESERVATION=""
# Account to use for the jobs. If left blank, the user's default
# account is used
ACCOUNT=""
# Email address to send notifications to. No notifications sent if
# blank
EMAIL=""
# What type of notifications to send - should be compatible with
# sbatch's requirements:
# BEGIN, END, FAIL, REQUEUE, ALL, TIME_LIMIT, TIME_LIMIT_90, TIME_LIMIT_80, and TIME_LIMIT_50
EMAIL_TYPE="ALL"

####################
# Times for individual slurm jobs
JOB_TIME_DEFAULT="12:00:00"
JOB_TIME_SPLIT_1934=""
JOB_TIME_SPLIT_SCIENCE=""
JOB_TIME_FLAG_1934=""
JOB_TIME_FLAG_SCIENCE=""
JOB_TIME_FIND_BANDPASS=""
JOB_TIME_APPLY_BANDPASS=""
JOB_TIME_AVERAGE_MS=""
JOB_TIME_CONT_IMAGE=""
JOB_TIME_SPECTRAL_SPLIT=""
JOB_TIME_SPECTRAL_APPLYCAL=""
JOB_TIME_SPECTRAL_CONTSUB=""
JOB_TIME_SPECTRAL_IMAGE=""
JOB_TIME_LINMOS=""
JOB_TIME_SOURCEFINDING=""
JOB_TIME_FITS_CONVERT=""
JOB_TIME_THUMBNAILS=""
JOB_TIME_CASDA_UPLOAD=""

####################
# Output directory for images, catalogues, tables, etc
# Should be *relative* to the base directory
OUTPUT=.

# Whether to overwrite the output data products, if they exist
# Options are true or false
CLOBBER=false

# Lustre filesystem striping to use within the current directory
LUSTRE_STRIPING=4

####################
# Locations of the executables. 
# If you have a local version of the ASKAPsoft codebase, and have
# defined ASKAP_ROOT, then you will use the executables from that
# tree. 
# Otherwise (the most common case for ACES members), the executables
# from the askapsoft module will be used.

if [ "$ASKAP_ROOT" != "" ]; then
    AIPSPATH=$ASKAP_ROOT/Code/Base/accessors/current
    mssplit=$ASKAP_ROOT/Code/Components/CP/pipelinetasks/current/apps/mssplit.sh
    cflag=$ASKAP_ROOT/Code/Components/CP/pipelinetasks/current/apps/cflag.sh
    cmodel=$ASKAP_ROOT/Code/Components/CP/pipelinetasks/current/apps/cmodel.sh
    cbpcalibrator=$ASKAP_ROOT/Code/Components/Synthesis/synthesis/current/apps/cbpcalibrator.sh
    ccalapply=$ASKAP_ROOT/Code/Components/Synthesis/synthesis/current/apps/ccalapply.sh
    cimager=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/cimager.sh
    ccalibrator=$ASKAP_ROOT/Code/Components/Synthesis/synthesis/current/apps/ccalibrator.sh
    ccontsubtract=$ASKAP_ROOT/Code/Components/Synthesis/synthesis/current/apps/ccontsubtract.sh
    simager=${ASKAP_ROOT}/Code/Components/CP/simager/current/apps/simager.sh
    linmos=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/linmos.sh
    selavy=${ASKAP_ROOT}/Code/Components/Analysis/analysis/current/apps/selavy.sh
    cimstat=${ASKAP_ROOT}/Code/Components/Analysis/analysis/current/apps/cimstat.sh
    mslist=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/mslist.sh
    image2fits=${ASKAP_ROOT}/3rdParty/casacore/casacore-2.0.3/install/bin/image2fits
    casdaupload=$ASKAP_ROOT/Code/Components/CP/pipelinetasks/current/apps/casdaupload.sh
    # export directives for slurm job files:
    exportDirective="#SBATCH --export=ASKAP_ROOT,AIPSPATH"
else
    mssplit=mssplit
    cflag=cflag
    cmodel=cmodel
    cbpcalibrator=cbpcalibrator
    ccalapply=ccalapply
    cimager=cimager
    ccalibrator=ccalibrator
    ccontsubtract=ccontsubtract
    simager=simager
    linmos=linmos
    selavy=selavy
    cimstat=cimstat
    mslist=mslist
    image2fits=image2fits
    casdaupload=casdaupload
    # export directives for slurm job files:
    exportDirective="#SBATCH --export=NONE"
fi

ASKAPSOFT_VERSION=""

####################
# Control flags

# Primary calibration
DO_1934_CAL=true
DO_SPLIT_1934=true
DO_FLAG_1934=true
DO_FIND_BANDPASS=true

# Calibration & imaging of the 'science' field.
DO_SCIENCE_FIELD=true
DO_SPLIT_SCIENCE=true
DO_FLAG_SCIENCE=true
DO_APPLY_BANDPASS=true
DO_AVERAGE_CHANNELS=true
DO_CONT_IMAGING=true
DO_SELFCAL=false
DO_SPECTRAL_IMAGING=false
DO_MOSAIC=true
DO_SOURCE_FINDING=false
DO_SOURCE_FINDING_MOSAIC=SETME
#
DO_CONVERT_TO_FITS=false
DO_MAKE_THUMBNAILS=false
DO_STAGE_FOR_CASDA=false

####################
# Input Scheduling Blocks (SBs)
# Location of the SBs
DIR_SB=/scratch2/askap/askapops/beta-scheduling-blocks
# SB with 1934-638 observation
SB_1934="SET_THIS"
MS_INPUT_1934=""
# SB with science observation
SB_SCIENCE="SET_THIS"
MS_INPUT_SCIENCE=""

####################
# Which beams to use.
BEAM_MIN=0
BEAM_MAX=8
BEAMLIST=""

# How many antennas
NUM_ANT=6

####################
##  BANDPASS CAL

# Base name for the 1934 measurement sets after splitting
MS_BASE_1934="1934_beam%b.ms"
# Channel range for splitting
CHAN_RANGE_1934="1-16416"
# Location of 1934-638, formatted for use in cbpcalibrator
DIRECTION_1934="[19h39m25.036, -63.42.45.63, J2000]"
# Name of the table for the bandpass calibration parameters
TABLE_BANDPASS=calparameters_1934_bp.tab
# Number of cycles used in cbpcalibrator
NCYCLES_BANDPASS_CAL=25
# Number of CPUs used for the cbpcalibrator job
NUM_CPUS_CBPCAL=400
# Value for the calibrate.scalenoise parameter for applying the
# bandpass solution
BANDPASS_SCALENOISE=false

# Whether to do dynamic flagging
FLAG_DO_DYNAMIC_AMPLITUDE_1934=true
# Dynamic threshold applied to amplitudes [sigma]
FLAG_THRESHOLD_DYNAMIC_1934=4.0
# Whether to apply a flat amplitude cut
FLAG_DO_FLAT_AMPLITUDE_1934=true
# Flat amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_1934=0.2
# Minimum amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_1934_LOW=0.0
# Baselines or antennas to flag in the 1934 case
ANTENNA_FLAG_1934=""
# Whether to flag autocorrelations for the 1934 data
FLAG_AUTOCORRELATION_1934=false

####################
## Science observation

# This allows selection of particular scans from the science
# observation. If this isn't needed, leave as a blank string.
SCAN_SELECTION_SCIENCE=""
# This allows selection of particular fields - BY NAME - from the
# science observation. If this isn't needed, leave as a blank string.
FIELD_SELECTION_SCIENCE=""
# Base name for the science observation measurement set
MS_BASE_SCIENCE=scienceObservation_beam%b.ms
# Name for the channel-averaged science measurement set (if blank, it
# will be set using MS_BASE_SCIENCE
MS_SCIENCE_AVERAGE=""
# Direction of the science field
DIRECTION_SCI=""

# Range of channels in science observation (used in splitting and averaging)
CHAN_RANGE_SCIENCE="1-16416"
# Number of channels to be averaged to create continuum measurement set
NUM_CHAN_TO_AVERAGE=54

# Whether to do dynamic flagging
FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE=true
# Dynamic threshold applied to amplitudes [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE=4.0
# Whether to apply a flat amplitude cut
FLAG_DO_FLAT_AMPLITUDE_SCIENCE=true
# Flat amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE=0.2
# Minimum amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=0.0
# Baselines or antennas to flag in the science data
ANTENNA_FLAG_SCIENCE=""
# Whether to flag autocorrelations for the science data
FLAG_AUTOCORRELATION_SCIENCE=false

# Data column in MS to use in cimager
DATACOLUMN=DATA
# Number of Taylor terms to create in MFS imaging
NUM_TAYLOR_TERMS=2
# Number of CPUs to use on each core in the continuum imaging
CPUS_PER_CORE_CONT_IMAGING=16

# base name for images: if IMAGE_BASE_CONT=i.blah then we'll get
# image.i.blah, image.i.blah.restored, psf.i.blah etc
IMAGE_BASE_CONT=i.cont
# number of pixels on the side of the images to be created
NUM_PIXELS_CONT=4096
# Size of the pixels in arcsec
CELLSIZE_CONT=10
# Frequency at which continuum image is made [Hz]
MFS_REF_FREQ=""
# Restoring beam: 'fit' will fit the PSF to determine the appropriate
# beam, else give a size
RESTORING_BEAM_CONT=fit

####################
# Gridding parameters for continuum imaging
GRIDDER_SNAPSHOT_IMAGING=true
GRIDDER_SNAPSHOT_WTOL=2600
GRIDDER_SNAPSHOT_LONGTRACK=true
GRIDDER_WMAX=2600
GRIDDER_NWPLANES=99
GRIDDER_OVERSAMPLE=4
GRIDDER_MAXSUPPORT=512

####################
# Cleaning parameters for continuum imaging
SOLVER=Clean
CLEAN_ALGORITHM=BasisfunctionMFS
CLEAN_MINORCYCLE_NITER=500
CLEAN_GAIN=0.5
CLEAN_SCALES="[0,3,10]"
CLEAN_THRESHOLD_MINORCYCLE="[30%, 0.9mJy]"
CLEAN_THRESHOLD_MAJORCYCLE=1mJy
CLEAN_NUM_MAJORCYCLES=2
# If true, this will write out intermediate images at the end of each
# major cycle
CLEAN_WRITE_AT_MAJOR_CYCLE=false
####################
# Parameters for preconditioning (A.K.A. weighting)
PRECONDITIONER_LIST="[Wiener, GaussianTaper]"
PRECONDITIONER_GAUSS_TAPER="[30arcsec, 30arcsec, 0deg]"
PRECONDITIONER_WIENER_ROBUSTNESS=0.5
PRECONDITIONER_WIENER_TAPER=""

####################
# Self-calibration parameters
# Interval [sec] over which to solve for self-calibration
SELFCAL_INTERVAL=10
# Number of loops of self-calibration
SELFCAL_NUM_LOOPS=5
# Should we keep the images from the intermediate selfcal loops?
SELFCAL_KEEP_IMAGES=true
# SNR threshold for detection with selavy in determining selfcal sources
SELFCAL_SELAVY_THRESHOLD=15
# Division of image for source-finding in selfcal
SELFCAL_SELAVY_NSUBX=6
SELFCAL_SELAVY_NSUBY=3
# Option to pass to the "Ccalibrator.normalisegains" parameter,
# indicating we want to approximate phase-only self-cal
SELFCAL_NORMALISE_GAINS=true
# Value for the calibrate.scalenoise parameter for applying the
# self-cal solution
SELFCAL_SCALENOISE=false

# name of the final gains calibration table
GAINS_CAL_TABLE=cont_gains_cal_beam%b.tab

# base name for images: if IMAGE_BASE_SPECTRAL=i.blah then we'll get
# image.i.blah, image.i.blah.restored, psf.i.blah etc
IMAGE_BASE_SPECTRAL=i.spectral
# number of pixels on the side of the images to be created
NUM_PIXELS_SPECTRAL=2048
# Size of the pixels in arcsec
CELLSIZE_SPECTRAL=10
# Frequency range for the spectral imager [Hz]
FREQ_RANGE_SPECTRAL="713.e6,1013.e6"

##############################
# Spectral-line imaging
# 
# For this, we repeat some of the parameters used for continuum
# imaging, with names that reflect the spectral-line nature...

### Preparation
# Whether to copy the data
DO_COPY_SL=false
# Channel range to copy
CHAN_RANGE_SL_SCIENCE=""
# Tile size for SL measurement set
TILENCHAN_SL=1
# Whether to apply a gains solution
DO_APPLY_CAL_SL=false
# Whether to subtract a continuum model
DO_CONT_SUB_SL=false

## Whether to contruct the continuum model via selavy & cmodel
#BUILD_MODEL_FOR_CONTSUB=true
CONTSUB_METHOD="Cmodel"
# Division of image for source-finding in continuum-subtraction
CONTSUB_SELAVY_NSUBX=6
CONTSUB_SELAVY_NSUBY=3
# Detection threshold for Selavy in building continuum model
CONTSUB_SELAVY_THRESHOLD=6

# Flux limit for cmodel
CONTSUB_MODEL_FLUX_LIMIT=10mJy

# Number of processors allocated to the spectral-line imaging
NUM_CPUS_SPECIMG_SCI=2000
# Number of processors per node for the spectral-line imaging
CPUS_PER_CORE_SPEC_IMAGING=20

# base name for image cubes: if IMAGE_BASE_SPECTRAL=i.blah then we'll
# get image.i.blah, image.i.blah.restored, psf.i.blah etc
IMAGE_BASE_SPECTRAL=i.cube

# Direction of the science field makes use of DIRECTION_SCI, as above

# number of spatial pixels on the side of the image cubes
NUM_PIXELS_SPECTRAL=2048
# Size of the pixels in arcsec
CELLSIZE_SPECTRAL=10
# Rest frequency for the cube
REST_FREQUENCY_SPECTRAL=HI

# Parameters for preconditioning (A.K.A. weighting) - allow these to
# be different to the continuum case
PRECONDITIONER_LIST_SPECTRAL="[Wiener, GaussianTaper]"
PRECONDITIONER_SPECTRAL_GAUSS_TAPER="[50arcsec, 50arcsec, 0deg]"
PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS=0.5
PRECONDITIONER_SPECTRAL_WIENER_TAPER=""

# Gridding parameters for spectral-line imaging
GRIDDER_SPECTRAL_SNAPSHOT_IMAGING=true
GRIDDER_SPECTRAL_SNAPSHOT_WTOL=2600
GRIDDER_SPECTRAL_SNAPSHOT_LONGTRACK=true
GRIDDER_SPECTRAL_WMAX=2600
GRIDDER_SPECTRAL_NWPLANES=99
GRIDDER_SPECTRAL_OVERSAMPLE=4
GRIDDER_SPECTRAL_MAXSUPPORT=512

# Cleaning parameters for spectral-line imaging
SOLVER_SPECTRAL=Clean
# default clean algorithm is Basisfunction, as we don't need the
# multi-frequency part that is used by BasisfunctionMFS
CLEAN_SPECTRAL_ALGORITHM=Basisfunction
CLEAN_SPECTRAL_MINORCYCLE_NITER=500
CLEAN_SPECTRAL_GAIN=0.5
CLEAN_SPECTRAL_SCALES="[0,3,10]"
CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE="[30%, 0.9mJy]"
CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE=1mJy
CLEAN_SPECTRAL_NUM_MAJORCYCLES=0
# If true, this will write out intermediate images at the end of each
# major cycle
CLEAN_SPECTRAL_WRITE_AT_MAJOR_CYCLE=false

# Whether to restore the spectral-line cubes
RESTORE_SPECTRAL=true
# Restoring beam to use in spectral-line cubes
RESTORING_BEAM_SPECTRAL=fit
# Reference channel for recording the restoring beam of the cube
RESTORING_BEAM_REFERENCE=mid
# Log file to record the restoring beam per channel
RESTORING_BEAM_LOG=beamLog.image.${IMAGE_BASE_SPECTRAL}.txt

##############################
# Linear Mosaicking
#
# Beam arrangement, used in linmos. If one of "diamond",
# "octagon",... then the positions are filled automatically.
# The name of the beam footprint. This needs to be recognised by footprint.py - see beamArrangements.sh
BEAM_FOOTPRINT_NAME="diamond"
# The position angle of the beam footprint
BEAM_FOOTPRINT_PA=0
# The pitch of the beams in the footprint
BEAM_PITCH=1.24
# This is the set of beam offsets used by linmos. This can be set manually instead of getting them from footprint.py
LINMOS_BEAM_OFFSETS=""
# Which frequency band are we in - determines beam arrangement (1,2,3,4 - 1 is lowest frequency)
FREQ_BAND_NUMBER=1
# Scale factor for beam arrangement, in format like '1deg'. Do not change if using the footprint.py names.
LINMOS_BEAM_SPACING="1deg"
# Reference beam for PSF
LINMOS_PSF_REF=0
# Cutoff for weights in linmos
LINMOS_CUTOFF=0.2

##############################
# Selavy source finder
#
# Signal-to-noise ratio threshold
SELAVY_SNR_CUT=5
# Whether to grow to a lower threshold
SELAVY_FLAG_GROWTH=true
# Growth threshold, in SNR
SELAVY_GROWTH_CUT=3
# Whether to use a variable threshold
SELAVY_VARIABLE_THRESHOLD=true
# Half-size of the box used to calculate the local threshold
SELAVY_BOX_SIZE=50
# How the processors subdivide the image
SELAVY_NSUBX=6
SELAVY_NSUBY=3


###############################
# Archiving-related parameters

# The image prefixes to be archived
IMAGE_LIST="image psf psf.image residual sensitivity"

# Whether to archive individual beam images 
ARCHIVE_BEAM_IMAGES=false

# OPAL project ID, for CASDA use
PROJECT_ID="AS031"

# Observation program description
OBS_PROGRAM="Commissioning"

# For making thumbnails, this is the python dictionary linking the
# size text to the figsize (in inches)
THUMBNAIL_SIZES_INCHES="{'big':16, 'sml':5}"
# Suffix for thumnail images - determines the image type
THUMBNAIL_SUFFIX="png"
# Grey-scale ranges for the thumbnails, in units of the overall image
# rms noise level
THUMBNAIL_GREYSCALE_MIN="-10"
THUMBNAIL_GREYSCALE_MAX="40"

# Write the READY file after casdaupload has finished?
WRITE_CASDA_READY=false

# Base directory for casdaupload output
CASDA_OUTPUT_DIR=/scratch2/casda/prd
