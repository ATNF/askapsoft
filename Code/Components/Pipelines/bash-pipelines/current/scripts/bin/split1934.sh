#!/bin/bash -l
#
# Launches a job to extract the appropriate beam & scan combination
# from the 1934-638 observation
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

ID_SPLIT_1934=""

# Get the name of the 1934 dataset, replacing any %b with the beam
# number if necessary
find1934MSnames

# Define the MS metadata file that we will store the mslist output for the new, merged MS.
# Store the name in MS_METADATA
find1934MSmetadataFile

if [ "$ms1934list" == "" ]; then 
    ms1934list="$msCal"
else
    ms1934list="$ms1934list,$msCal"
fi

if [ "${CHAN_RANGE_1934}" == "" ]; then
    # No selection of channels
    chanSelection=""
    chanParam="channel     = 1-${NUM_CHAN_1934}"
else
    # CHAN_RANGE_SCIENCE gives global channel range - pass this to getMatchingMS.py
    chanSelection="-c ${CHAN_RANGE_1934}"
    chanParam="channel     = ${CHAN_RANGE_1934}
"
fi

# Allow splitting of user-specified TimeRange to derive bandpass from: 
# Very limited (but useful) feature that will allow users to process 
# longTrack 1934
# Default is NOT to split out 1934 data. 
timeParamBeg_1934="# TimeBeg specified: beginning of msdata" 
timeParamEnd_1934="# TimeEnd specified: end of msdata." 
if [ "$SPLIT_TIME_START_1934" != "" ];
then
	timeParamBeg_1934="timebegin       =   ${SPLIT_TIME_START_1934}" 
fi
if [ "$SPLIT_TIME_END_1934" != "" ];
then
	timeParamEnd_1934="timeend         =   ${SPLIT_TIME_END_1934}" 
fi

DO_IT=$DO_SPLIT_1934

if [ -e "${OUTPUT}/${msCal}" ]; then
    if [ "${CLOBBER}" != "true" ]; then
        # If we aren't clobbering files, don't run anything
        if [ "${DO_IT}" == "true" ]; then
            echo "MS ${msCal} exists, so not splitting for beam ${BEAM}"
        fi
        DO_IT=false
    else
        # If we are clobbering files, removing the existing one, but
        # only if we are going to be running the job
        if [ "${DO_IT}" == "true" ]; then
            rm -rf "${OUTPUT}/${msCal}"
            rm -f "${FLAG_1934_CHECK_FILE}"
        fi
    fi
fi

if [ "${DO_IT}" == "true" ]; then

    # Define the input MS. If there is a single MS in the archive
    # directory, just use MS_INPUT_1934. If there is more than one, we
    # assume it is split per beam. Unlike the science case, we don't
    # compare the number of beams with the number of MSs, we just use
    # the list as presented. We add the beam number to the root MS
    # name.

    if [ $numMS1934 -eq 1 ]; then
        inputMS=${MS_INPUT_1934}
    else
        msroot=$(for ms in ${msnames1934}; do echo $ms; done | sed -e 's/[0-9]*[0-9].ms//g' | uniq)
        inputMS=$(echo $msroot $BEAM | awk '{printf "%s%d.ms\n",$1,$2}')
        if [ ! -e "${inputMS}" ]; then
            echo "ERROR - wanted to use $inputMS as input for beam $BEAM"
            echo "      -  but it does not exist! Exiting."
            exit 1
        fi
    fi
        
    if [ "${DO_FIND_BANDPASS}" == "true" ] && [ -e "${TABLE_BANDPASS}" ]; then
        # If we are splitting and the user wants to find the bandpass,
        # remove any existing bandpass table so that we will be able
        # to create a new one
        echo "Removing the bandpass table so we can recompute"
        rm -rf "${TABLE_BANDPASS}" "${TABLE_BANDPASS}".smooth
    fi

    if [ -e "${FLAG_1934_CHECK_FILE}" ]; then
        echo "Removing check file for flagging"
        rm -f "${FLAG_1934_CHECK_FILE}"
    fi

    # If we have >1 MS and the number differs from the number of
    # beams, we need to do merging. 
    if [ "${NEED_TO_MERGE_CAL}" == "true" ]; then

        verb="splitting and merging"
        setJob splitmerge_1934 splitmerge
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPLIT_1934}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-splitMergeCal-b${BEAM}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

finalMS=${msCal}
sb1934dir=$sb1934dir
BEAM=$BEAM
chanSelection="$chanSelection"
inputMSlist=\$(\${PIPELINEDIR}/getMatchingMS.py -d \${sb1934dir} -b \$BEAM \$chanSelection)

if [ "\${inputMSlist}" == "" ]; then
    if [ "\${chanSelection}" == "" ]; then
        echo "No MS found for beam \$BEAM in directory \${sb1934dir}"
    else
        echo "No MS found for beam \$BEAM and channel selection \${chanSelection} in directory \${sb1934dir}"
    fi
    echo "Exiting with error code 1"
    exit 1
fi

echo "Datasets to be split and merged are:"
echo "\$inputMSlist"

COUNT=0
mergelist=""
for dataset in \${inputMSlist}; do

    ms=\$(echo \$dataset | awk -F':' '{print \$1}')
    chanRange=\$(echo \$dataset | awk -F':' '{print \$2}')

    sedstr="s/\.ms/_\${COUNT}\.ms/g"
    outputms=\$(echo \$finalMS | sed -e \$sedstr)
    mergelist="\$mergelist \$outputms"
    parset="${parsets}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}_\${COUNT}.in"
    cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = \${ms}

# Output measurement set
# Default: <no default>
outputvis   = \${outputms}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
channel     = \${chanRange}
# If you wanted to split out data from a specified timeWindow for BandpassCalibration
# (Particularly useful to quickly derive bandpass from a longTrack 1934 obs)
$timeParamBeg_1934
$timeParamEnd_1934


# Beam selection via beam ID
# Select an individual beam
beams        = [${BEAM}]

# Scan selection for the 1934-638 observation. Assume the scan ID matches the beam ID
scans        = [${BEAM}]

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
# Make the tile size 54 channels, as that is what we will average over
stman.tilenchan   = ${TILE_NCHAN_1934}
EOFINNER

    log="${logs}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}_\${COUNT}.log"

    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
    err=\$?
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname}_\${COUNT} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
    ((COUNT++))
done

commandLineFlags="-c ${TILE_NCHAN_1934} -o \${finalMS} \${mergelist}"
log="${logs}/mergeMS_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

echo "Running msmerge with arguments: \${commandLineFlags}" > \$log
STARTTIME=\$(date +%FT%T)
NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" ${msmerge} \${commandLineFlags} >> "\${log}"
err=\$?
rejuvenate \${finalMS}
echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

# Make the metadata file for the new MS
metadataFile="${MS_METADATA_CAL}"
NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mslist} --full \${finalMS} 1>& "\${metadataFile}"

# Remove interim MSs if required.
purgeMSs=${PURGE_INTERIM_MS_1934}
if [ "\${purgeMSs}" == "true" ]; then
    for ms in \${mergelist}; do
        lfs find \$ms -type f -print0 | xargs -0 munlink
        find \$ms -type l -delete
        find \$ms -depth -type d -empty -delete
    done
fi

EOFOUTER
    else

        verb="splitting"
        setJob split_1934 split
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPLIT_1934}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-split1934-b${BEAM}-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

parset="${parsets}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
cat > "\$parset" <<EOFINNER
# Input measurement set
# Default: <no default>
vis         = ${inputMS}

# Output measurement set
# Default: <no default>
outputvis   = ${msCal}

# The channel range to split out into its own measurement set
# Can be either a single integer (e.g. 1) or a range (e.g. 1-300). The range
# is inclusive of both the start and end, indexing is one-based.
# Default: <no default>
$chanParam
$timeParamBeg_1934
$timeParamEnd_1934

# Beam selection via beam ID
# Select just a single beam for this obs
beams        = [${BEAM}]

# Scan selection for the 1934-628 observation. Assume the scan ID matches the beam ID
scans        = [${BEAM}]

# Set a larger bucketsize
stman.bucketsize  = ${BUCKET_SIZE}
stman.tilenchan   = ${TILE_NCHAN_1934}
EOFINNER

log="${logs}/split_1934_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${mssplit} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msCal}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    fi
    
    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
	ID_SPLIT_1934=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_SPLIT_1934}" "Run ${verb} of beam ${BEAM} of 1934-638 observation"
    else
	echo "Would run ${verb} of ${BEAM} of 1934-638 observation with slurm file $sbatchfile"
    fi

fi


