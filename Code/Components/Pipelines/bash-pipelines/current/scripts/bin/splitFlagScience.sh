#!/usr/bin/env bash
#
# Launches a job to extract the science observation from the
# appropriate measurement set, then flag the data in two passes, one
# with a dynamic threshold and the second with a flat amplitude cut to
# remove any remaining spikes.
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

ID_SPLIT_SCI=""
ID_FLAG_SCI=""

DO_IT=$DO_SPLIT_SCIENCE

if [ -e ${OUTPUT}/${msSci} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msSci} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${OUTPUT}/${msSci}
            rm -f ${FLAG_CHECK_FILE}
            rm -f ${BANDPASS_CHECK_FILE}
        fi
    fi
fi

if [ $DO_IT == true ]; then

    if [ "$SCAN_SELECTION_SCIENCE" == "" ]; then
	scanParam="# No scan selection done"
    else
        if [ `echo ${SCAN_SELECTION_SCIENCE} | awk -F'[' '{print NF}'` -gt 1 ]; then
	    scanParam="scans        = ${SCAN_SELECTION_SCIENCE}"
        else
            scanParam="scans        = [${SCAN_SELECTION_SCIENCE}]"
        fi
    fi

    if [ "$FIELD_SELECTION_SCIENCE" == "" ]; then
	fieldParam="# No field selection done"
    else
	if [ `echo ${FIELD_SELECTION_SCIENCE} | awk -F'[' '{print NF}'` -gt 1 ]; then
            fieldParam="fieldnames   = ${FIELD_SELECTION_SCIENCE}"
        else
            fieldParam="fieldnames   = [${FIELD_SELECTION_SCIENCE}]"
        fi
    fi

    sbatchfile=$slurms/split_science_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=splitSci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-splitSci-b${BEAM}-%j.out

cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/split_science_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${MS_INPUT_SCIENCE}

# Output measurement set
# Default: <no default>
outputvis   = ${msSci}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_SCIENCE}

# Beam selection via beam ID
# Select an individual beam
beams        = [${BEAM}]

# Scan selection for the science observation
$scanParam

# Field selection for the science observation
$fieldParam

# Set a larger bucketsize
stman.bucketsize  = 65536
# Make the tile size 54 channels, as that is what we will average over
stman.tilenchan   = ${NUM_CHAN_TO_AVERAGE}
EOFINNER

log=${logs}/split_science_beam${BEAM}_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 ${mssplit} -c \${parset} > \${log}
err=\$?
NUM_CPUS=1
extractStats \${log} \${SLURM_JOB_ID} \${err} splitScience_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
	ID_SPLIT_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SPLIT_SCI} "Splitting beam ${BEAM} of science observation"
    else
	echo "Would run splitting ${BEAM} of science observation with slurm file $sbatchfile"
    fi

fi

###########################################

DO_IT=$DO_FLAG_SCIENCE
if [ -e $FLAG_CHECK_FILE ]; then
    if [ $DO_IT == true ]; then
        echo "Flagging for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

     if [ "$ANTENNA_FLAG_SCIENCE" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rules           = [rule1]
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_SCIENCE}"
    fi
    
   
    sbatchfile=$slurms/flag_science_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=flagSci${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-flagSci-b${BEAM}-%j.out

cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/cflag_dynamic_science_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

# Amplitude based flagging with dynamic thresholds
#  This finds a statistical threshold in the spectrum of each
#  time-step, then applies the same threshold level to the integrated
#  spectrum at the end.
Cflag.amplitude_flagger.enable           = true
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE}
Cflag.amplitude_flagger.integrateSpectra = true
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE}

${antennaFlagging}
EOFINNER

log=${logs}/cflag_dynamic_science_beam${BEAM}_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 ${cflag} -c \${parset} > \${log}
err=\$?
NUM_CPUS=1
extractStats \${log} \${SLURM_JOB_ID} \${err} flagScienceDynamic_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $FLAG_CHECK_FILE
fi

parset=${parsets}/cflag_amp_science_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level, to remove
#   any remaining spikes
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE}
Cflag.amplitude_flagger.low             = 0.
EOFINNER

log=${logs}/cflag_amp_science_beam${BEAM}_\${SLURM_JOB_ID}.log

aprun -n 1 -N 1 ${cflag} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${SLURM_JOB_ID} \${err} flagScienceAmp_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
	ID_FLAG_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_FLAG_SCI} "Flagging beam ${BEAM} of science observation"
    else
	echo "Would run flagging beam ${BEAM} for science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
