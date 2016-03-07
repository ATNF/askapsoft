#!/usr/bin/env bash
#
# Launches a job to extract the appropriate beam from the 1934-638
# observation, then flag the data in two passes, one with a dynamic
# threshold and the second with a flat amplitude cut to remove any
# remaining spikes.
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

ID_SPLIT_1934=""
ID_FLAG_1934=""

# Get the name of the 1934 dataset, replacing any %b with the beam
# number if necessary
find1934MSnames

if [ "$ms1934list" == "" ]; then 
    ms1934list="$msCal"
else
    ms1934list="$ms1934list,$msCal"
fi

DO_IT=$DO_SPLIT_1934

if [ -e ${OUTPUT}/${msCal} ]; then
    if [ $CLOBBER == false ]; then
        # If we aren't clobbering files, don't run anything
        if [ $DO_IT == true ]; then
            echo "MS ${msCal} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ $DO_IT == true ]; then
            rm -rf ${OUTPUT}/${msCal}
            rm -f ${FLAG_1934_CHECK_FILE}
        fi
    fi
fi

if [ $DO_IT == true ]; then

    if [ "$ANTENNA_FLAG_1934" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rules           = [rule1]
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_1934}"
    fi
    
    sbatchfile=$slurms/split_1934_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=splitCal${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-split1934-b${BEAM}-%j.out

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/split_1934_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${MS_INPUT_1934}

# Output measurement set
# Default: <no default>
outputvis   = ${msCal}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = ${CHAN_RANGE_1934}

# Beam selection via beam ID
# Select just a single beam for this obs
beams        = [${BEAM}]

# Scan selection for the 1934-638 observation. Assume the scan ID matches the beam ID
scans        = [${BEAM}]

# Set a larger bucketsize
stman.bucketsize  = 65536
stman.tilenchan   = ${NUM_CHAN_TO_AVERAGE}
EOFINNER

log=${logs}/split_1934_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${mssplit} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} split1934_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    
    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
	ID_SPLIT_1934=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SPLIT_1934} "Splitting beam ${BEAM} of 1934-638 observation"
    else
	echo "Would run splitting ${BEAM} of 1934-638 observation with slurm file $sbatchfile"
    fi

fi

###########################################

DO_IT=$DO_FLAG_1934
if [ -e $FLAG_1934_CHECK_FILE ]; then
    if [ $DO_IT == true ]; then
        echo "Flagging for beam $BEAM of calibrator observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    if [ "$ANTENNA_FLAG_1934" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rules           = [rule1]
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_1934}"
    fi
    
    sbatchfile=$slurms/flag_1934_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=flagCal${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-flag1934-b${BEAM}-%j.out

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/cflag_dynamic_1934_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msCal}

# Amplitude based flagging
Cflag.amplitude_flagger.enable           = true
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_1934}
Cflag.amplitude_flagger.integrateSpectra = true
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_1934}

${antennaFlagging}
EOFINNER

log=${logs}/cflag_dynamic_1934_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${cflag} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} flag1934Dynamic_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $FLAG_1934_CHECK_FILE
fi

parset=${parsets}/cflag_amp_1934_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msCal}

# Amplitude based flagging
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_1934}
Cflag.amplitude_flagger.low             = 0.
EOFINNER


log=${logs}/cflag_amp_1934_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${cflag} -c \${parset} > \${log}
err=\$?
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} flag1934Amp_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_1934"`
        ID_FLAG_1934=`sbatch $DEP $sbatchfile | awk '{print $4}'`
        recordJob ${ID_FLAG_1934} "Splitting and flagging 1934-638, beam $BEAM"
        FLAG_CBPCAL_DEP=`addDep "$FLAG_CBPCAL_DEP" "$ID_FLAG_1934"`
    else
        echo "Would run splitting & flagging of 1934-638, beam $BEAM, with slurm file $sbatchfile"
    fi

    echo " "

fi
