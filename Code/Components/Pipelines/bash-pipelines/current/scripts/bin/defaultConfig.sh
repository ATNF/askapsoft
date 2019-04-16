#!/bin/bash -l
#
# The complete set of user-defined parameters, along with their
# default values. All parameters defined herein can be set in the
# input file, and any value given there will override the default
# value set here.
#
# @copyright (c) 2017 CSIRO
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
# Special constraints to apply. Not needed for galaxy queue, but may
# be useful for other clusters
CONSTRAINT=""
# Email address to send notifications to. No notifications sent if
# blank
EMAIL=""
# What type of notifications to send - should be compatible with
# sbatch's requirements:
# BEGIN, END, FAIL, REQUEUE, ALL, TIME_LIMIT, TIME_LIMIT_90, TIME_LIMIT_80, and TIME_LIMIT_50
EMAIL_TYPE="ALL"

# Location to copy pipeline errors to, for when jobs fail
FAILURE_DIRECTORY="/group/askaprt/processing/pipeline-errors"

####################
# Times for individual slurm jobs
JOB_TIME_DEFAULT="24:00:00"
JOB_TIME_SPLIT_1934=""
JOB_TIME_SPLIT_SCIENCE=""
JOB_TIME_FLAG_1934=""
JOB_TIME_FLAG_SCIENCE=""
JOB_TIME_FIND_BANDPASS=""
JOB_TIME_APPLY_BANDPASS=""
JOB_TIME_AVERAGE_MS=""
JOB_TIME_MSCONCAT_SCI_AV=""
JOB_TIME_MSCONCAT_SCI_SPECTRAL=""
JOB_TIME_CONT_IMAGE=""
JOB_TIME_CONT_SELFCAL=""
JOB_TIME_CONT_APPLYCAL=""
JOB_TIME_CONTCUBE_IMAGE=""
JOB_TIME_SPECTRAL_SPLIT=""
JOB_TIME_SPECTRAL_APPLYCAL=""
JOB_TIME_SPECTRAL_CONTSUB=""
JOB_TIME_SPECTRAL_IMAGE=""
JOB_TIME_SPECTRAL_IMCONTSUB=""
JOB_TIME_LINMOS=""
JOB_TIME_SOURCEFINDING_CONT=""
JOB_TIME_SOURCEFINDING_SPEC=""
JOB_TIME_DIAGNOSTICS=""
JOB_TIME_FITS_CONVERT=""
JOB_TIME_THUMBNAILS=""
JOB_TIME_VALIDATE=""
JOB_TIME_CASDA_UPLOAD=""

####################
# Output directory for images, catalogues, tables, etc
# Should be *relative* to the base directory
OUTPUT=.

# Whether to overwrite the output data products, if they exist
# Options are true or false
CLOBBER=false

# Lustre filesystem stripe count to use within the current directory
LUSTRE_STRIPING=4

# Lustre filesystem stripe size - default=1MB
LUSTRE_STRIPE_SIZE=1048576

# Storage manager bucket size - I/O quantum for MS writing
BUCKET_SIZE=1048576

# Number of channels in the tile for the new MS we create through merging or splitting
TILE_NCHAN_SCIENCE=54
TILE_NCHAN_1934=54
TILENCHAN_AV=18

# Remove interim MSs when running local merging
PURGE_INTERIM_MS_SCI=true
PURGE_INTERIM_MS_1934=true

# If true, turn on profiling for the imaging jobs (only cimager and imager have this option)
USE_PROFILING=false

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
    altimager=${ASKAP_ROOT}/Code/Components/CP/askap_imager/current/apps/imager.sh
    linmos=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/linmos.sh
    linmosMPI=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/linmos-mpi.sh
    selavy=${ASKAP_ROOT}/Code/Components/Analysis/analysis/current/apps/selavy.sh
    cimstat=${ASKAP_ROOT}/Code/Components/Analysis/analysis/current/apps/cimstat.sh
    mslist=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/mslist.sh
    msmerge=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/msmerge.sh
    msconcat=${ASKAP_ROOT}/Code/Components/Synthesis/synthesis/current/apps/msconcat.sh
    image2fits=${ASKAP_ROOT}/3rdParty/casacore/casacore-2.0.3/install/bin/image2fits
    makeThumbnails=${ASKAP_ROOT}/Code/Components/Analysis/evaluation/current/install/bin/makeThumbnailImage.py
    casdaupload=$ASKAP_ROOT/Code/Components/CP/pipelinetasks/current/apps/casdaupload.sh
    # export directives for slurm job files:
    exportDirective="#SBATCH --export=ASKAP_ROOT,AIPSPATH"
else
    mssplit=mssplit
    msmerge=msmerge
    msconcat=msconcat
    cflag=cflag
    cmodel=cmodel
    cbpcalibrator=cbpcalibrator
    ccalapply=ccalapply
    cimager=cimager
    ccalibrator=ccalibrator
    ccontsubtract=ccontsubtract
    simager=simager
    altimager=imager
    linmos=linmos
    linmosMPI="linmos-mpi"
    selavy=selavy
    cimstat=cimstat
    mslist=mslist
    image2fits=image2fits
    makeThumbnails=makeThumbnailImage.py
    casdaupload=casdaupload
    # export directives for slurm job files:
    exportDirective="#SBATCH --export=NONE"
fi

# User can select a particular version of the askapsoft module
ASKAPSOFT_VERSION=""

# Base directory for modules, that will be used by pipeline and slurm
# scripts
ASKAP_MODULE_DIR="/group/askap/modulefiles"

# User can use the acesops module (true), or their own nominated ACES
# directory (false)
USE_ACES_OPS=true

# Version of the acesops module to use. Leave blank for the default
ACESOPS_VERSION=""

####################
# Control flags

# Primary calibration
DO_1934_CAL=true
DO_SPLIT_1934=true
SPLIT_TIME_START_1934=""
SPLIT_TIME_END_1934=""
DO_FLAG_1934=true
DO_FIND_BANDPASS=true
DO_SPLIT_TIMEWISE=true
DO_SPLIT_TIMEWISE_SERIAL=true
SPLIT_INTERVAL_MINUTES=60

# Calibration & imaging of the 'science' field.
DO_SCIENCE_FIELD=true
DO_SPLIT_SCIENCE=true
DO_FLAG_SCIENCE=true
DO_APPLY_BANDPASS=true
DO_AVERAGE_CHANNELS=true
DO_CONT_IMAGING=true
DO_SELFCAL=true
MULTI_JOB_SELFCAL=true
DO_APPLY_CAL_CONT=true
DO_CONTCUBE_IMAGING=false
DO_SPECTRAL_PROCESSING=false
DO_COPY_SL=false
DO_APPLY_CAL_SL=true
DO_CONT_SUB_SL=true
DO_SPECTRAL_IMAGING=true
DO_SPECTRAL_IMSUB=true
DO_MOSAIC=true
DO_MOSAIC_FIELDS=true
DO_SOURCE_FINDING_CONT=""
DO_SOURCE_FINDING_SPEC=""
DO_SOURCE_FINDING_BEAMWISE=false
DO_SOURCE_FINDING_FIELD_MOSAICS=false
DO_ALT_IMAGER=true
DO_ALT_IMAGER_CONT=""
DO_ALT_IMAGER_CONTCUBE=""
DO_ALT_IMAGER_SPECTRAL=""
#
DO_DIAGNOSTICS=true
DO_CONVERT_TO_FITS=true
DO_MAKE_THUMBNAILS=true
DO_VALIDATION_SCIENCE=true
DO_STAGE_FOR_CASDA=false

####################
# Input Scheduling Blocks (SBs)
# Location of the SBs
DIR_SB=/astro/askaprt/askapops/askap-scheduling-blocks
# SB with 1934-638 observation
SB_1934=""
MS_INPUT_1934=""
# SB with science observation
SB_SCIENCE=""
MS_INPUT_SCIENCE=""

# Set to true if the dataset being processed is from BETA observations
IS_BETA=false

# Set to not true if you know the schedblock & footprint services are offline
USE_CLI=true

####################
# Which beams to use.
NUM_BEAMS_FOOTPRINT=36
NUM_BEAMS_FOOTPRINT_CAL=36
BEAM_MIN=0
BEAM_MAX=35
BEAMLIST=""

####################
# Image output type
IMAGETYPE_CONT=fits
IMAGETYPE_CONTCUBE=fits
IMAGETYPE_SPECTRAL=fits

####################
##  BANDPASS CAL

# Base name for the 1934 measurement sets after splitting
MS_BASE_1934="1934_SB%s_%b.ms"
# Channel range for splitting - defaults to full set of channels in MS
CHAN_RANGE_1934=""
# Location of 1934-638, formatted for use in cbpcalibrator
DIRECTION_1934="[19h39m25.036, -63.42.45.63, J2000]"
# Name of the table for the bandpass calibration parameters
TABLE_BANDPASS="calparameters_1934_bp_SB%s.tab"
# Number of cycles used in cbpcalibrator
NCYCLES_BANDPASS_CAL=50
# Number of CPUs (cores) used for the cbpcalibrator job
NUM_CPUS_CBPCAL=216
# Value for the calibrate.scalenoise parameter for applying the
# bandpass solution
BANDPASS_SCALENOISE=false
# Limit the data selection for bandpass solving to a minimum UV distance [m]
BANDPASS_MINUV=200
# Reference antenna to be used in the cbpcalibrator task
BANDPASS_REFANTENNA=1

# Smoothing of the bandpass table.
# This is achieved by either the ACES tool plot_caltable.py (which
# also plots the cal solutions), or with Wasim Raja's
# smooth_bandpass.py tool.

# Whether to smooth the bandpass
DO_BANDPASS_SMOOTH=true
# User-defined string for passing optional arguments to the bandpass_smooth_tool:
# By default we DO NOT USE this string. If supplied by user in the config file, 
# all optional arguments will be overwritten by this string. 
BANDPASS_SMOOTH_ARG_STRING=""
# Which method to use - either "plot_caltable" or "smooth_bandpass"
BANDPASS_SMOOTH_TOOL="plot_caltable"

# The following are parameters for plot_caltable
# Whether to run plot_caltable.py to produce plots
DO_BANDPASS_PLOT=true
# If true, smooth the amplitudes. If false, smooth real & imaginary
BANDPASS_SMOOTH_AMP=true
# If true, only smooth outlier points
BANDPASS_SMOOTH_OUTLIER=true
# polynomial order (if >= 0) or window size (if <0) to use when smoothing bandpass
BANDPASS_SMOOTH_FIT=1
# The threshold level for fitting bandpass
BANDPASS_SMOOTH_THRESHOLD=1.0

# The following are parameters for smooth_bandpass
# The polynomial order for the fit
BANDPASS_SMOOTH_POLY_ORDER=""
# The harmonic order for the fit
BANDPASS_SMOOTH_HARM_ORDER=""
# The number of windows to divide the spectrum into for the moving fit
BANDPASS_SMOOTH_N_WIN=""
# The width (in channels) of the Gaussian Taper function to remove
# high-frequency components
BANDPASS_SMOOTH_N_TAPER=""
# The number of iterations for Fourier-interpolation across flagged
# points
BANDPASS_SMOOTH_N_ITER=""
# If > 0, the smooth solutions will be derived by fitting only 54x${BANDPASS_SMOOTH_F54} 
# number of points. The beam-forming interval (where jumps are seen) can be 54 (normal 
# obs mode) or nx54 (for zoom-modes). 
BANDPASS_SMOOTH_F54=""

# Whether to apply the bandpass solution to the 1934 dataset itself
DO_APPLY_BANDPASS_1934=true
# If true, keep the raw bandpass datasets separate to the calibrated
# versions
KEEP_RAW_1934_DATA=true

# Parallelism for applying bandpass to full-size science datasets
#  Don't change unless you know what you are doing!
NUM_CORES_CAL_APPLY=19


####################
##  Flagging the bandpass calibrator dataset
#
# Whether to do dynamic flagging
FLAG_DO_DYNAMIC_AMPLITUDE_1934=true
# Dynamic threshold applied to amplitudes [sigma]
FLAG_THRESHOLD_DYNAMIC_1934=4.0
# Whether to apply a dynamic threshold to integrated spectra
FLAG_DYNAMIC_1934_INTEGRATE_SPECTRA=true
# Dynamic threshold applied to amplitudes in integrated spectra mode  [sigma]
FLAG_THRESHOLD_DYNAMIC_1934_SPECTRA=4.0
# Whether to apply a dynamic threshold to integrated times
FLAG_DYNAMIC_1934_INTEGRATE_TIMES=false
# Dynamic threshold applied to amplitudes in integrated times mode [sigma]
FLAG_THRESHOLD_DYNAMIC_1934_TIMES=4.0
#
# Whether to do Stokes-V flagging
FLAG_DO_STOKESV_1934=true
# Whether to use robust statistics for Stokes-V flagging
FLAG_USE_ROBUST_STATS_STOKESV_1934=true
# Threshold applied to amplitudes in Stokes-V flagging [sigma]
FLAG_THRESHOLD_STOKESV_1934=4.0
# Whether to apply a Stokes-V threshold to integrated spectra
FLAG_STOKESV_1934_INTEGRATE_SPECTRA=true
# Threshold applied to amplitudes in integrated spectra mode of
# Stokes-V flagging  [sigma]
FLAG_THRESHOLD_STOKESV_1934_SPECTRA=4.0
# Whether to apply a Stokes-V threshold to integrated times
FLAG_STOKESV_1934_INTEGRATE_TIMES=false
# Threshold applied to amplitudes in integrated times mode of Stokes-V
# flagging [sigma]
FLAG_THRESHOLD_STOKESV_1934_TIMES=4.0
#
# Whether to apply a flat amplitude cut
FLAG_DO_FLAT_AMPLITUDE_1934=false
# Flat amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_1934=0.2
# Minimum amplitude threshold applied [hardware units - before calibration]
FLAG_THRESHOLD_AMPLITUDE_1934_LOW=""
# Baselines or antennas to flag in the 1934 data
ANTENNA_FLAG_1934=""
# Channel range(s) to flag in the 1934 data
CHANNEL_FLAG_1934=""
# Time range(s) to flag in the 1934 data
TIME_FLAG_1934=""
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
MS_BASE_SCIENCE="scienceData_SB%s_%b.ms"
# Name for the channel-averaged science measurement set (if blank, it
# will be set using MS_BASE_SCIENCE)
MS_SCIENCE_AVERAGE=""
# Direction of the science field - defaults to centre of MS
DIRECTION_SCI=""
# Make the images at the centres of the beams, rather than the same
# image centre for each beam
IMAGE_AT_BEAM_CENTRES=true

# Range of channels in science observation (used in splitting and
# averaging)  - defaults to full set of channels in MS
CHAN_RANGE_SCIENCE=""
# Number of channels to be averaged to create continuum measurement set
NUM_CHAN_TO_AVERAGE=54

# Whether to do dynamic flagging
FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE=true
# Dynamic threshold applied to amplitudes  [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE=4.0
# Whether to apply a dynamic threshold to integrated spectra
FLAG_DYNAMIC_INTEGRATE_SPECTRA=true
# Dynamic threshold applied to amplitudes in integrated spectra mode  [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA=4.0
# Whether to apply a dynamic threshold to integrated times
FLAG_DYNAMIC_INTEGRATE_TIMES=false
# Dynamic threshold applied to amplitudes in integrated times mode [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES=4.0
# Flagging based on Elevation limits [el in degrees]
ELEVATION_FLAG_SCIENCE_LOW=""
ELEVATION_FLAG_SCIENCE_HIGH=""
# Whether to do Stokes-V flagging
FLAG_DO_STOKESV_SCIENCE=true
# Whether to use robust statistics for Stokes-V flagging
FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE=true
# Threshold applied to amplitudes in Stokes-V flagging  [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE=4.0
# Whether to apply a Stokes-V threshold to integrated spectra
FLAG_STOKESV_INTEGRATE_SPECTRA=true
# Threshold applied to amplitudes in integrated spectra mode for
# Stokes-V flagging  [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA=4.0
# Whether to apply a Stokes-V threshold to integrated times
FLAG_STOKESV_INTEGRATE_TIMES=false
# Threshold applied to amplitudes in integrated times mode for
# Stokes-V flagging [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES=4.0
#
# Whether to apply a flat amplitude cut
FLAG_DO_FLAT_AMPLITUDE_SCIENCE=false
# Flat amplitude threshold applied [calibrated flux units]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE=10.
# Minimum amplitude threshold applied [calibrated flux units]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW=""
# Baselines or antennas to flag in the science data
ANTENNA_FLAG_SCIENCE=""
# Channel range(s) to flag in the science data
CHANNEL_FLAG_SCIENCE=""
# Time range(s) to flag in the science data
TIME_FLAG_SCIENCE=""
# Whether to flag autocorrelations for the science data
FLAG_AUTOCORRELATION_SCIENCE=false

# Whether to remove the full-resolution dataset after averaging
#     (We set this to true by default, but if spectral imaging is
#      desired we change to false)
PURGE_FULL_MS=true

# Run flagging after averaging, as well as after bandpass application
FLAG_AFTER_AVERAGING=true
# Whether to do dynamic flagging on the averaged data
FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE_AV=true
# Dynamic threshold applied to amplitudes of averaged data [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE_AV=4.0
# Whether to apply a dynamic threshold to integrated spectra on averaged data
FLAG_DYNAMIC_INTEGRATE_SPECTRA_AV=true
# Dynamic threshold applied to amplitudes in integrated spectra mode  [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE_SPECTRA_AV=4.0
# Whether to apply a dynamic threshold to integrated times on averaged data
FLAG_DYNAMIC_INTEGRATE_TIMES_AV=false
# Dynamic threshold applied to amplitudes in integrated times mode [sigma]
FLAG_THRESHOLD_DYNAMIC_SCIENCE_TIMES_AV=4.0
#
# Whether to do Stokes-V flagging on the averaged data
FLAG_DO_STOKESV_SCIENCE_AV=true
# Whether to use robust statistics for Stokes-V flagging
FLAG_USE_ROBUST_STATS_STOKESV_SCIENCE_AV=true
# Threshold applied to amplitudes of averaged data for Stokes-V
# flagging [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE_AV=4.0
# Whether to apply a Stokes-V threshold to integrated spectra on averaged data
FLAG_STOKESV_INTEGRATE_SPECTRA_AV=true
# Threshold applied to amplitudes in integrated spectra mode for
# Stokes-V flagging  [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE_SPECTRA_AV=4.0
# Whether to apply a Stokes-V threshold to integrated times on averaged data
FLAG_STOKESV_INTEGRATE_TIMES_AV=false
# Threshold applied to amplitudes in integrated times mode for
# Stokes-V flagging [sigma]
FLAG_THRESHOLD_STOKESV_SCIENCE_TIMES_AV=4.0
#
# Whether to apply a flat amplitude cut to the averaged data
FLAG_DO_FLAT_AMPLITUDE_SCIENCE_AV=false
# Flat amplitude threshold applied to the averaged data [calibrated flux units]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE_AV=10.
# Minimum amplitude threshold applied to the averaged data [calibrated flux units]
FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW_AV=""
# Channel range(s) to flag in the averaged science data
CHANNEL_FLAG_SCIENCE_AV=""
# Time range(s) to flag in the averaged science data
TIME_FLAG_SCIENCE_AV=""

######################
# AOflagger options

# Global switch, plus switches for individual flagging tasks. Setting
# the global switch will override the individual values
FLAG_WITH_AOFLAGGER=""
FLAG_1934_WITH_AOFLAGGER=false
FLAG_SCIENCE_WITH_AOFLAGGER=false
FLAG_SCIENCE_AV_WITH_AOFLAGGER=false

# Strategy files. Setting the global one overrides the individual
# values.
AOFLAGGER_STRATEGY=""
AOFLAGGER_STRATEGY_1934=""
AOFLAGGER_STRATEGY_SCIENCE=""
AOFLAGGER_STRATEGY_SCIENCE_AV=""

# Verbose output (-v option for aoflagger) - global parameter
AOFLAGGER_VERBOSE=true

# Read mode for AOflagger - either "auto", "direct", "indirect",
# "memory". These trigger the following respective command-line
# options: -auto-read-mode, -direct-read, -indirect-read, -memory-read
AOFLAGGER_READ_MODE="auto"

# Allow use of uvw values
AOFLAGGER_UVW=false

######################
# Msconcat of TimeWiseSplit Data
NUM_CORES_MSCONCAT_SCI_AV=1
NUM_CORES_MSCONCAT_SCI_SPECTRAL=1
NPPN_MSCONCAT_SCI_AV=1
NPPN_MSCONCAT_SCI_SPECTRAL=1
######################
# Imaging

# Data column in MS to use in cimager
DATACOLUMN=DATA
# Number of Taylor terms to create in MFS imaging
NUM_TAYLOR_TERMS=2
# Number of CPUs to use on each core in the continuum imaging
CPUS_PER_CORE_CONT_IMAGING=6
# Total number of cores to use for the continuum imaging. Leave blank
# to have one core for each of nworkergroups*nchannels (for cimager)
# or nworkergroups*nchannels/nchanpercore (for imager), plus a master.
NUM_CPUS_CONTIMG_SCI=""
# Whether to put the master on a node of its own for the continuum imaging
FAT_NODE_CONT_IMG=true

# The Cimager.Channels parameter, to be used when NUM_CPUS_CONTIMG_SCI
# is given. Cimager.Channels is left out of the imaging parset if this
# is not given.
CHANNEL_SELECTION_CONTIMG_SCI=""

# base name for images: if IMAGE_BASE_CONT=i.blah then we'll get
# image.i.blah, image.i.blah.restored, psf.i.blah etc
IMAGE_BASE_CONT="i.SB%s.cont"
# number of pixels on the side of the images to be created
NUM_PIXELS_CONT=6144
# Size of the pixels in arcsec
CELLSIZE_CONT=2
# Frequency at which continuum image is made [Hz]
MFS_REF_FREQ=""
# Restoring beam: 'fit' will fit the PSF to determine the appropriate
# beam, else give a size
RESTORING_BEAM_CONT=fit
# Cutoff in determining support for the fit to the PSF
RESTORING_BEAM_CUTOFF_CONT=0.5

###########################
# parameters from the new (alt) imager
# number of channels each core will process
NCHAN_PER_CORE=12
# specify this separately for the continuum-cube imaging
NCHAN_PER_CORE_CONTCUBE=3
# the spectral line imager needs its own otherwise we lose some flexibility
NCHAN_PER_CORE_SL=64
# store the visibilities in shared memory.
# this will give a performance boost at the expense of memory usage
USE_TMPFS=false
# where is the shared memory mounted
TMPFS="/dev/shm"
# Frequency frame for the output spectral cube - one of "topo", "bary", "lsrk"
FREQ_FRAME_SL=bary
# Output channels for the spectral cube - needs to be [number,start,width]
OUTPUT_CHANNELS_SL=""
# local solver - distribute the minor cycle - each channel is solved individually
# this mimics simager behaviour
# automatically set to true in the spectral imaging
DO_LOCAL_SOLVER=false
# How many sub-cubes to write out.
# This improves performance of the imaging - and also permits parallelisation
# of the LINMOS step
NUM_SPECTRAL_WRITERS=""
# Whether to write out a single file in the case of writing to FITS
ALT_IMAGER_SINGLE_FILE=true

# Same for continuum cubes
NUM_SPECTRAL_WRITERS_CONTCUBE=""
ALT_IMAGER_SINGLE_FILE_CONTCUBE=true


####################
# Gridding parameters for continuum imaging
GRIDDER_SNAPSHOT_IMAGING=false
# Actual parameters used, which depend on whether snapshot imaging is used
GRIDDER_WMAX=""
GRIDDER_MAXSUPPORT=""
GRIDDER_NWPLANES=""
# Defaults, with and without snapshot imaging
GRIDDER_WMAX_SNAPSHOT=2600
GRIDDER_MAXSUPPORT_SNAPSHOT=512
GRIDDER_WMAX_NO_SNAPSHOT=30000
GRIDDER_MAXSUPPORT_NO_SNAPSHOT=1024
GRIDDER_NWPLANES_SNAPSHOT=99
GRIDDER_NWPLANES_NO_SNAPSHOT=257
# Other gridding parameters that don't change with snapshot status
GRIDDER_SNAPSHOT_WTOL=2600
GRIDDER_SNAPSHOT_LONGTRACK=true
GRIDDER_SNAPSHOT_CLIPPING=0.01
GRIDDER_OVERSAMPLE=5

####################
# Cleaning parameters for continuum imaging
SOLVER=Clean
CLEAN_ALGORITHM=BasisfunctionMFS
CLEAN_MINORCYCLE_NITER="[400,800]"
CLEAN_GAIN=0.2
CLEAN_PSFWIDTH=256
CLEAN_SCALES="[0,3,10]"
CLEAN_THRESHOLD_MINORCYCLE="[30%, 0.5mJy, 0.03mJy]"
# If true, this will write out intermediate images at the end of each
# major cycle
CLEAN_WRITE_AT_MAJOR_CYCLE=false

# solution type for continuum cleaning - either MAXCHISQ or MAXBASE
CLEAN_SOLUTIONTYPE="MAXBASE"

# Array-capable self-calibration parameters
#   These parameters can be given as either a single value (eg. "300")
#   which is replicated for all self-cal loops, or as an array
#   (eg. "[1800,900,300]"), allowing a different value for each loop.
# If no self-calibration is used, we just use the first element
#
# The number of major cycles in the deconvolution
CLEAN_NUM_MAJORCYCLES="[5,10]"
# The maximum residual to stop the major-cycle deconvolution (if not
# reached, or negative, CLEAN_NUM_MAJORCYCLES cycles are used)
CLEAN_THRESHOLD_MAJORCYCLE="0.035mJy"


####################
# Parameters for preconditioning (A.K.A. weighting)
PRECONDITIONER_LIST="[Wiener]"
PRECONDITIONER_GAUSS_TAPER="[10arcsec, 10arcsec, 0deg]"
PRECONDITIONER_WIENER_ROBUSTNESS=-0.5
PRECONDITIONER_WIENER_TAPER=""
# Parameters for preconditioning for the restore solver alone
RESTORE_PRECONDITIONER_LIST=""
RESTORE_PRECONDITIONER_GAUSS_TAPER="[10arcsec, 10arcsec, 0deg]"
RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS=-2
RESTORE_PRECONDITIONER_WIENER_TAPER=""


####################
# Self-calibration parameters
#
# Method to present self-cal model: via a model image ("Cmodel")
# via a components parset ("Components"), or using the
# continuum-imaging clean model ("CleanModel")
SELFCAL_METHOD="Cmodel"
# Number of loops of self-calibration
SELFCAL_NUM_LOOPS=1
# Should we keep the images from the intermediate selfcal loops?
SELFCAL_KEEP_IMAGES=true
# Should we make full-field mosaics of each loop iteration?
MOSAIC_SELFCAL_LOOPS=false
# Division of image for source-finding in selfcal
SELFCAL_SELAVY_NSUBX=6
SELFCAL_SELAVY_NSUBY=3
# Weights threshold to apply when source-finding in self-calibration
# mode - a fraction of the peak weight, below which pixels are not
# detected.
SELFCAL_SELAVY_WEIGHTSCUT=0.95
# Type of fitting to apply within Selavy for the self-calibration.
SELFCAL_SELAVY_FIT_TYPE="full"
# Value for the calibrate.scalenoise parameter for applying the
# self-cal solution
SELFCAL_SCALENOISE=false
# SNR limit for components to go into self-cal model image
SELFCAL_COMPONENT_SNR_LIMIT=10
# Flux limit for cmodel - fall back value if SELFCAL_COMPONENT_SNR_LIMIT not given
SELFCAL_MODEL_FLUX_LIMIT=10uJy
# Whether to use the number of Gaussians taken from initial estimate
SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS=true
# If SELFCAL_SELAVY_GAUSSIANS_FROM_GUESS=false, this is how many
# Gaussians to use
SELFCAL_SELAVY_NUM_GAUSSIANS=1
# Reference antenna to use in self-calibration. Should be antenna
# number, 0 - nAnt-1 that matches antenna numbering in MS
SELFCAL_REF_ANTENNA=""
# Reference gains to use in self-calibration - something like
# gain.g11.0.0
SELFCAL_REF_GAINS=""

# Array-capable self-calibration parameters
#   These parameters can be given as either a single value (eg. "300")
#   which is replicated for all self-cal loops, or as an array
#   (eg. "[1800,900,300]"), allowing a different value for each loop.
#
# Interval [sec] over which to solve for self-calibration
SELFCAL_INTERVAL="[200,200]"
# SNR threshold for detection with selavy in determining selfcal sources
SELFCAL_SELAVY_THRESHOLD=8
# Option to pass to the "Ccalibrator.normalisegains" parameter,
# indicating we want to approximate phase-only self-cal
SELFCAL_NORMALISE_GAINS=true
# Limit the data selection for imaging to a minimum UV distance [m]
CIMAGER_MINUV=0
# Limit the data selection for calibration to a minimum UV distance [m]
CCALIBRATOR_MINUV=0
# Limit the data selection for imaging to a minimum UV distance [m] (ignored if 0)
CIMAGER_MAXUV=0
# Limit the data selection for calibration to a minimum UV distance [m] (ignored if 0)
CCALIBRATOR_MAXUV=0

# name of the final gains calibration table
GAINS_CAL_TABLE="cont_gains_cal_SB%s_%b.tab"
KEEP_RAW_AV_MS=true

# Shift position offsets - precomputed, and added to position in final
# self-cal selavy catalogue
DO_POSITION_OFFSET=false
# Position offsets in arcsec
RA_POSITION_OFFSET=0
DEC_POSITION_OFFSET=0

###################
# Parameters for continuum cube imaging

# base name for continuum cubes: if IMAGE_BASE_CONT=i.blah then we'll
# get image.i.blah, image.i.blah.restored, psf.i.blah etc
# Polarisations will replace the .i. in the image name using the list
# in CONTCUBE_POLARISATIONS
IMAGE_BASE_CONTCUBE="i.SB%s.contcube"

# Image size for continuum cubes (spatial size) [pixels]
NUM_PIXELS_CONTCUBE=4096

# Size of the pixels for the continuum cubes [arcsec]
CELLSIZE_CONTCUBE=""

# List of polarisations to make continuum cubes for. The lower-case
# version of these will go in the image name.
CONTCUBE_POLARISATIONS="I"

# Set if there needs to be a rest frequency recorded in the continuum cubes
REST_FREQUENCY_CONTCUBE=""
# Restoring beam for continuum cubes: 'fit' will fit the PSF to
# determine the appropriate beam, else give a size
RESTORING_BEAM_CONTCUBE=fit
# Reference channel for recording the restoring beam of the cube
RESTORING_BEAM_CONTCUBE_REFERENCE=mid
# Cutoff in determining support for the fit to the PSF
RESTORING_BEAM_CUTOFF_CONTCUBE=0.5

# Number of processors for continuum-cube imaging.
# Leave blank to fit to number of channels
NUM_CPUS_CONTCUBE_SCI=""
# Number of processors per node for the spectral-line imaging
CPUS_PER_CORE_CONTCUBE_IMAGING=8
# Number of processors for continuum-cube mosaicking.
# Leave blank to fit to number of channels
NUM_CPUS_CONTCUBE_LINMOS=""

# Cleaning parameters for spectral-line imaging
# Which solver to use
SOLVER_CONTCUBE=Clean
CLEAN_CONTCUBE_ALGORITHM=BasisfunctionMFS
CLEAN_CONTCUBE_MINORCYCLE_NITER=600
CLEAN_CONTCUBE_GAIN=0.2
CLEAN_CONTCUBE_PSFWIDTH=256
CLEAN_CONTCUBE_SCALES="[0,3,10]"
CLEAN_CONTCUBE_SOLUTIONTYPE="MAXBASE"
CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE="[40%, 0.5mJy, 0.05mJy]"
CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE=0.06mJy
CLEAN_CONTCUBE_NUM_MAJORCYCLES=3
# If true, this will write out intermediate images at the end of each
# major cycle
CLEAN_CONTCUBE_WRITE_AT_MAJOR_CYCLE=false

##############################
# Spectral-line imaging
#
# For this, we repeat some of the parameters used for continuum
# imaging, with names that reflect the spectral-line nature...

### Preparation
# Channel range to copy when DO_COPY_SL is turned on
CHAN_RANGE_SL_SCIENCE=""
# Tile size for SL measurement set
TILENCHAN_SL=18

# Method to present self-cal model: via a model image ("Cmodel"),
# via a components parset ("Components"), or using the
# continuum-imaging clean model ("CleanModel")
CONTSUB_METHOD="Cmodel"
# Division of image for source-finding in continuum-subtraction
CONTSUB_SELAVY_NSUBX=6
CONTSUB_SELAVY_NSUBY=3
# Detection threshold for Selavy in building continuum model
CONTSUB_SELAVY_THRESHOLD=6
# Whether to use contiguous pixels in the islands
CONTSUB_SELAVY_FLAG_ADJACENT=true
# If above is false, this is the spatial threshold to join islands
CONTSUB_SELAVY_SPATIAL_THRESHOLD=5

# Flux limit for cmodel
CONTSUB_MODEL_FLUX_LIMIT=10uJy

# Number of processors allocated to the spectral-line imaging
NUM_CPUS_SPECIMG_SCI=200
# Number of processors per node for the spectral-line imaging
CPUS_PER_CORE_SPEC_IMAGING=20
# Number of processors for spectral-line mosaicking.
# Leave blank to scale according to number of channels per core
NUM_CPUS_SPECTRAL_LINMOS=""
# Number of channels handled by each core in the spectral-line
# mosaicking. Will determine total number of cores based on number of
# channels to be mosaicked.
NCHAN_PER_CORE_SPECTRAL_LINMOS=8

# base name for image cubes: if IMAGE_BASE_SPECTRAL=i.blah then we'll
# get image.i.blah, image.i.blah.restored, psf.i.blah etc
IMAGE_BASE_SPECTRAL="i.SB%s.cube"

# Direction of the science field makes use of DIRECTION_SCI, as above

# number of spatial pixels on the side of the image cubes
NUM_PIXELS_SPECTRAL=1024
# Size of the pixels in arcsec
CELLSIZE_SPECTRAL=8
# Rest frequency for the cube
REST_FREQUENCY_SPECTRAL=HI

# maximum UV spacing for spectral imaging - ignored if 0
SPECTRAL_IMAGE_MAXUV=2000
# minimum UV spacing for spectral imaging
SPECTRAL_IMAGE_MINUV=0

# Parameters for preconditioning (A.K.A. weighting) - allow these to
# be different to the continuum case
PRECONDITIONER_LIST_SPECTRAL="[Wiener,GaussianTaper]"
PRECONDITIONER_SPECTRAL_GAUSS_TAPER="[20arcsec, 20arcsec, 0deg]"
PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS=0.5
PRECONDITIONER_SPECTRAL_WIENER_TAPER=""

# Gridding parameters for spectral-line imaging
GRIDDER_SPECTRAL_SNAPSHOT_IMAGING=false
# Actual parameters used, which depend on whether snapshot imaging is used
GRIDDER_SPECTRAL_WMAX=""
GRIDDER_SPECTRAL_MAXSUPPORT=""
# Defaults, with and without snapshot imaging
GRIDDER_SPECTRAL_WMAX_SNAPSHOT=2600
GRIDDER_SPECTRAL_MAXSUPPORT_SNAPSHOT=512
GRIDDER_SPECTRAL_WMAX_NO_SNAPSHOT=30000
GRIDDER_SPECTRAL_MAXSUPPORT_NO_SNAPSHOT=1024
# Other gridding parameters that don't change with snapshot status
GRIDDER_SPECTRAL_SNAPSHOT_WTOL=2600
GRIDDER_SPECTRAL_SNAPSHOT_LONGTRACK=true
GRIDDER_SPECTRAL_SNAPSHOT_CLIPPING=0.01
GRIDDER_SPECTRAL_NWPLANES=257
GRIDDER_SPECTRAL_OVERSAMPLE=4

# Cleaning parameters for spectral-line imaging
SOLVER_SPECTRAL=Clean
CLEAN_SPECTRAL_ALGORITHM=BasisfunctionMFS
CLEAN_SPECTRAL_MINORCYCLE_NITER=800
CLEAN_SPECTRAL_GAIN=0.2
CLEAN_SPECTRAL_PSFWIDTH=256
CLEAN_SPECTRAL_SCALES="[0,3,10,30]"
CLEAN_SPECTRAL_SOLUTIONTYPE="MAXBASE"
CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE="[45%, 3.5mJy, 0.5mJy]"
CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE=0.5mJy
CLEAN_SPECTRAL_NUM_MAJORCYCLES=3
# If true, this will write out intermediate images at the end of each
# major cycle
CLEAN_SPECTRAL_WRITE_AT_MAJOR_CYCLE=false

# Whether to restore the spectral-line cubes
RESTORE_SPECTRAL=true
# Restoring beam to use in spectral-line cubes
RESTORING_BEAM_SPECTRAL=fit
# Reference channel for recording the restoring beam of the cube
RESTORING_BEAM_REFERENCE=mid
# Cutoff in determining support for the fit to the PSF
RESTORING_BEAM_CUTOFF_SPECTRAL=0.5

# Image-based continuum subtraction
# Which ACES script to use - only "robust_contsub.py" or
# "contsub_im.py" accepted
SPECTRAL_IMSUB_SCRIPT=robust_contsub.py
# Defaults that apply to both scripts
# Verbose output
SPECTRAL_IMSUB_VERBOSE=true
#
# Defaults for the robust_contsub.py script
# Threshold [sigma] to mask outliers prior to fitting ('threshold' parameter)
SPECTRAL_IMSUB_THRESHOLD=2.0
# Order of polynomial to fit ('fit_order' parameter)
SPECTRAL_IMSUB_FIT_ORDER=2
# Only use every nth channel ('n_every' parameter)
SPECTRAL_IMSUB_CHAN_SAMPLING=1
# Log every nth channel ('log_every' parameter)
SPECTRAL_IMSUB_LOG_SAMPLING=1
#
# Defaults for the contsub_im.py script
# Half-width of the Savitzky-Golay filter [channels]
SPECTRAL_IMSUB_SG_FILTERWIDTH=200
# Bin width for binning spectrum before continuum subtraction
SPECTRAL_IMSUB_SG_BINWIDTH=4

##############################
# Linear Mosaicking & beam locations
#
# Reference footprint rotation angle. This is meant to stand in for
# the scheduling block parameter
# common.target.src%d.footprint.rotation, but if given, it will
# over-ride that value.
# If not given, footprint.rotation is used. If that is not present,
# it has the same effect as setting to zero.
FOOTPRINT_PA_REFERENCE=""
#
# Beam arrangement, used in linmos. These specifications are only used
# in the IS_BETA case, or if the footprint listed in the schedblock is
# not understood by the footprint service. In this case, we use the
# ACES tool footprint.py, and the name needs to be recognised by
# footprint.py. See findBeamCentres.sh for details.
BEAM_FOOTPRINT_NAME="diamond"
# The position angle of the beam footprint
BEAM_FOOTPRINT_PA=0
# The pitch of the beams in the footprint
BEAM_PITCH=""
# This is the set of beam offsets used by linmos. This can be set manually instead of getting them from footprint.py
LINMOS_BEAM_OFFSETS=""
# Which frequency band are we in - determines beam arrangement (1,2,3,4 - 1 is lowest frequency)
# This is over-ridden with BEAM_PITCH.
FREQ_BAND_NUMBER=""
# Scale factor for beam arrangement, in format like '1deg'. Do not change if using the footprint.py names.
LINMOS_BEAM_SPACING="1deg"
# Reference beam for PSF
LINMOS_PSF_REF=0
# Cutoff for weights in linmos
LINMOS_CUTOFF=0.2
# The argument used for linmos.weighttype when mosaicking all beams of a given footprint. Can be either "Combined" or "FromPrimaryBeamModel".
LINMOS_SINGLE_FIELD_WEIGHTTYPE="Combined"

##############################
# Selavy source finder - continuum
#
# Signal-to-noise ratio threshold
SELAVY_SNR_CUT=5
# Flux threshold - leave blank to use SNR
SELAVY_FLUX_THRESHOLD=""
# Whether to grow to a lower threshold
SELAVY_FLAG_GROWTH=true
# Growth threshold, in SNR
SELAVY_GROWTH_CUT=3
# Growth flux threshold - leave blank if using SNR
SELAVY_GROWTH_THRESHOLD=""
# Cutoff in the weights for source-detection
SELAVY_WEIGHTS_CUTOFF=""
# Whether to use a variable threshold
SELAVY_VARIABLE_THRESHOLD=true
# Half-size of the box used to calculate the local threshold
SELAVY_BOX_SIZE=50
# How the processors subdivide the image
SELAVY_NSUBX=6
SELAVY_NSUBY=3
SELAVY_OVERLAPX=0
SELAVY_OVERLAPY=0
#
# Whether to use the continuum cube to find the spectral index &
# curvature
USE_CONTCUBE_FOR_SPECTRAL_INDEX=false
# If using contcube, this is the number of terms to solve for (1=just
# I_0, 2 = I_0 & alpha, 3 = I_0, alpha, beta)
SELAVY_NUM_SPECTRAL_TERMS=3
#
# Whether to use contiguous pixels in the islands
SELAVY_FLAG_ADJACENT=true
# If above is false, this is the spatial threshold to join islands
SELAVY_SPATIAL_THRESHOLD=5

##############################
# Run the continuum validation script following source finding
DO_CONTINUUM_VALIDATION=true

# Run the validation for individual beam images
VALIDATE_BEAM_IMAGES=false

# Location to copy validation directories to
VALIDATION_ARCHIVE_DIR="/group/askap/ValidationReportsArchive"

# Whether to remove the .csv files when copying the validation
# directories
REMOVE_VALIDATION_CSV=true

##############################
# Selavy source finder - polarisation
#
# Whether to include the RM synthesis in the continuum sourcefinding
DO_RM_SYNTHESIS=false
# Output base name for the spectra
SELAVY_POL_OUTPUT_BASE=pol
# Whether to write the spectra as individual files
SELAVY_POL_WRITE_SPECTRA=true
# Whether to write the Faraday Dispersion Function as a complex-valued
# spectrum (true) or as two real-valued spectra for amplitude & phase
SELAVY_POL_WRITE_COMPLEX_FDF=false
# The full width of the box used in spectral extraction
SELAVY_POL_BOX_WIDTH=5
# The area (in multiples of the beam) for the noise extraction
SELAVY_POL_NOISE_AREA=50
# Whether to use robust statistics in the noise calculation
SELAVY_POL_ROBUST_STATS=true
# The type of weighting in the RM synthesis
SELAVY_POL_WEIGHT_TYPE=variance
# The type of Stokes-I model spectrum
SELAVY_POL_MODEL_TYPE=taylor
# For SELAVY_POL_MODEL_TYPE=poly, this is the order of the polynomial
SELAVY_POL_MODEL_ORDER=3
# The signal-to-noise threshold to accept a RM synthesis detection
SELAVY_POL_SNR_THRESHOLD=8
# The signal-to-noise threshold above which debiasing is performed
SELAVY_POL_DEBIAS_THRESHOLD=5
# The number of Faraday depth channels in RM synthesis
SELAVY_POL_NUM_PHI_CHAN=30
# Width of the Faraday depth channels [rad/m2]
SELAVY_POL_DELTA_PHI=5
# Central Faraday depth of the FDF
SELAVY_POL_PHI_ZERO=0


##############################
# Selavy source finder - spectral-line
#
# Allow user to specify number of cores per node. If blank, we work it
# out based on number of requested cores
CPUS_PER_CORE_SELAVY_SPEC=""
# Signal-to-noise ratio threshold
SELAVY_SPEC_SNR_CUT=8
# Flux threshold - leave blank to use SNR
SELAVY_SPEC_FLUX_THRESHOLD=""
# Whether to grow to a lower threshold
SELAVY_SPEC_FLAG_GROWTH=true
# Growth threshold, in SNR
SELAVY_SPEC_GROWTH_CUT=3
# Growth flux threshold - leave blank if using SNR
SELAVY_SPEC_GROWTH_THRESHOLD=""
# Cutoff in the weights for source-detection
SELAVY_SPEC_WEIGHTS_CUTOFF=""
#
# Preprocessing
# Smoothing:
SELAVY_SPEC_FLAG_SMOOTH=true
# Type of smoothing - 'spectral' or 'spatial'
SELAVY_SPEC_SMOOTH_TYPE=spectral
# Spectral smoothing hanning width (channels)
SELAVY_SPEC_HANN_WIDTH=5
# Spatial smoothing Gaussian kernel - either a single value ("3") or a
# vector of three values ("[4,3,45]")
SELAVY_SPEC_SPATIAL_KERNEL=3
# Wavelet reconstruction
SELAVY_SPEC_FLAG_WAVELET=false
# Dimension to do the wavelet reconstruction
SELAVY_SPEC_RECON_DIM=1
# Signal-to-noise for wavelet thresholding
SELAVY_SPEC_RECON_SNR=4
# Minimum scale for inclusion in reconstruction (1=lowest)
SELAVY_SPEC_RECON_SCALE_MIN=1
# Maximum scale for inclusion in reconstruction (0 means all scales)
SELAVY_SPEC_RECON_SCALE_MAX=0
#
# Type of searching to be done - 'spectral' or 'spatial'
SELAVY_SPEC_SEARCH_TYPE=spatial
# Whether to use a variable threshold
SELAVY_SPEC_VARIABLE_THRESHOLD=true
# Half-size of the box used to calculate the local threshold
SELAVY_SPEC_BOX_SIZE=35
# How the processors subdivide the image
SELAVY_SPEC_NSUBX=6
SELAVY_SPEC_NSUBY=9
SELAVY_SPEC_NSUBZ=18
SELAVY_SPEC_OVERLAPX=0
SELAVY_SPEC_OVERLAPY=0
SELAVY_SPEC_OVERLAPZ=20
#
# Limits on sizes of reported sources
SELAVY_SPEC_MIN_PIX=5
SELAVY_SPEC_MIN_CHAN=5
SELAVY_SPEC_MAX_CHAN=2592
#
# Whether to use the optimiseMask option
SELAVY_SPEC_OPTIMISE_MASK=false
#

###############################
# Archiving-related parameters

# The image prefixes to be archived
IMAGE_LIST="image psf psf.image residual sensitivity"

# Whether we should archive the full-resolution (spectral) measurement
# sets
ARCHIVE_SPECTRAL_MS=true

# Whether to archive individual beam images
ARCHIVE_BEAM_IMAGES=false
# Whether to archive mosaics of self-calibration loops
ARCHIVE_SELFCAL_LOOP_MOSAICS=false
# Whether to archive the mosaicked images of each field
ARCHIVE_FIELD_MOSAICS=false

# TELESCOP FITS header keyword
TELESCOP_KEYWORD="ASKAP"

# OPAL project ID, for CASDA use
PROJECT_ID="AS033"

# Observation program description
OBS_PROGRAM="Commissioning"

# For making thumbnails, this is the size value given to figsize (in inches)
THUMBNAIL_SIZE_INCHES="16,5"
# For making thumbnails, this is the corresponding string for the sizes
THUMBNAIL_SIZE_TEXT="large,small"

# Suffix for thumnail images - determines the image type
THUMBNAIL_SUFFIX="png"
# Grey-scale ranges for the thumbnails, in units of the overall image
# rms noise level
THUMBNAIL_GREYSCALE_MIN="-10"
THUMBNAIL_GREYSCALE_MAX="40"

# Use absolute paths to files in the XML file made for CASDA? This
# allows them to remain in situ, rather than being copied. It will
# make them globally readable.
CASDA_USE_ABSOLUTE_PATHS=true

# Write the READY file after casdaupload has finished?
WRITE_CASDA_READY=false

# Transition the scheduling block status to PENDINGARCHIVE once the
# READY file has been written
TRANSITION_SB=false

# JIRA project to which SB annotations should be sent
JIRA_ANNOTATION_PROJECT="ASKAPOBS"

# Base directory for casdaupload output
CASDA_UPLOAD_DIR=/group/casda/prd

# Delay between slurm jobs that poll CASDA output directory for the DONE file
POLLING_DELAY_SEC=1800
# Maximum time we wait to check for DONE file
MAX_POLL_WAIT_TIME=172800
