#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, using the BasisfunctionMFS solver.
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

# Define the Cimager parset and associated parameters
. "${PIPELINEDIR}/getContinuumCimagerParams.sh"

ID_CONTIMG_SCI=""

DO_IT="$DO_CONT_IMAGING"

if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
    theimager=$altimager
else
    theimager=$cimager
fi

imageCode=restored
setImageProperties cont

if [ "${CLOBBER}" != "true" ] && [ -e "${imageName}" ]; then
    if [ "${DO_IT}" == "true" ]; then
        echo "Image ${imageName} exists, so not running continuum imaging for beam ${BEAM}"
    fi
    DO_IT=false
fi

if [ "${DO_IT}" == "true" ]; then

    if [ "${FAT_NODE_CONT_IMG}" == "true" ]; then
        # Need to take account of the master on a node by itself.
        NUM_NODES_IMAGING=$(echo $NUM_CPUS_CONTIMG_SCI $CPUS_PER_CORE_CONT_IMAGING | awk '{nworkers=$1-1; if (nworkers%$2==0) workernodes=nworkers/$2; else workernodes=int(nworkers/$2)+1; print workernodes+1}')
    else
        NUM_NODES_IMAGING=$(echo $NUM_CPUS_CONTIMG_SCI $CPUS_PER_CORE_CONT_IMAGING | awk '{if ($1%$2==0) print $1/$2; else print int($1/$2)+1}')
    fi

    setJob science_continuumImage cont
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --nodes=${NUM_NODES_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contImaging-%j.out"

${askapsoftModuleCommands}
FAT_NODE_CONT_IMG=${FAT_NODE_CONT_IMG}
nodeDistribution="--ntasks-per-node=\${NPPN} "
if [ "\${FAT_NODE_CONT_IMG}" == "true" ]; then

    nodelist=\$SLURM_JOB_NODELIST
    
    # Make arrangements to put task 0 (master) of imager on the first node, 
    # and spread the rest out over the other nodes:
    
    newlist=\$(hostname)
    icpu=0
    for node in \$(scontrol show hostnames \$nodelist); do
        if [[ "\$node" != "\$(hostname)" ]]; then
            for proc in \$(seq 1 ${CPUS_PER_CORE_CONT_IMAGING}); do
    	    icpu=\$((icpu+1))
    	    if [[ "\$icpu" -lt "${NUM_CPUS_CONTIMG_SCI}" ]]; then 
    		newlist=\$newlist,\$node
     	    fi
            done
        fi
    done
    echo "NodeList: "\$nodelist
    echo "NewList: "\$newlist

    nodeDistribution="--nodelist=\$newlist --distribution=arbitrary"

fi

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

# Parameters that can vary with self-calibration loop number (which is
#     zero in this case)
CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=(${CLEAN_THRESHOLD_MAJORCYCLE_ARRAY[@]})
CLEAN_NUM_MAJORCYCLES_ARRAY=(${CLEAN_NUM_MAJORCYCLES_ARRAY[@]})
CIMAGER_MINUV_ARRAY=(${CIMAGER_MINUV_ARRAY[@]})
CCALIBRATOR_MINUV_ARRAY=(${CCALIBRATOR_MINUV_ARRAY[@]})
CIMAGER_MAXUV_ARRAY=(${CIMAGER_MAXUV_ARRAY[@]})
CCALIBRATOR_MAXUV_ARRAY=(${CCALIBRATOR_MAXUV_ARRAY[@]})
CLEAN_ALGORITHM_ARRAY=(${CLEAN_ALGORITHM_ARRAY[@]})
CLEAN_MINORCYCLE_NITER_ARRAY=(${CLEAN_MINORCYCLE_NITER_ARRAY[@]})
CLEAN_GAIN_ARRAY=(${CLEAN_GAIN_ARRAY[@]})
CLEAN_PSFWIDTH_ARRAY=(${CLEAN_PSFWIDTH_ARRAY[@]})
CLEAN_SCALES_ARRAY=(${CLEAN_SCALES_ARRAY[@]})
CLEAN_THRESHOLD_MINORCYCLE_ARRAY=(${CLEAN_THRESHOLD_MINORCYCLE_ARRAY[@]})

LOOP=0

cimagerSelfcalLoopParams
dataSelectionSelfcalLoop

parset="${parsets}/science_imaging_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
cat > "\$parset" <<EOFINNER
${cimagerParams}
#
\${loopParams}
\${dataSelectionParamsIm}
#
# Do not apply any calibration
Cimager.calibrate                               = false
EOFINNER

log="${logs}/science_imaging_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

NCORES=${NUM_CPUS_CONTIMG_SCI}
NPPN=${CPUS_PER_CORE_CONT_IMAGING}
srun --export=ALL --ntasks=\${NCORES} \${nodeDistribution} $theimager ${PROFILE_FLAG} -c "\$parset" >> "\$log"
err=\$?

# Handle the profiling files
doProfiling=${USE_PROFILING}
if [ "\${doProfiling}" == "true" ]; then
    dir=Profiling/Beam${BEAM}
    mkdir -p \$dir
    mv profile.*.${imageBase}* \${dir}
fi

for im in *.${imageBase}*; do
    rejuvenate "\$im"
done
rejuvenate "${OUTPUT}"/"${msSciAv}"
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
	if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then 
            DEP=$(addDep "$DEP" "$ID_MSCONCAT_SCI_AV")
	fi
	ID_CONTIMG_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_CONTIMG_SCI}" "Make a continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
        FLAG_IMAGING_DEP=$(addDep "$FLAG_IMAGING_DEP" "$ID_CONTIMG_SCI")
    else
	echo "Would make a continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
