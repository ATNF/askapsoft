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
if [ "${DO_APPLY_CAL_CONT}" == "true" ]; then
    msToUse=$msSciAvCal
else
    msToUse=$msSciAvFull
fi

ID_CONTPOL_IMG_SCI=""

if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
    theimager=$altimager
else
    theimager=$cimager
fi

for stokes_now in $CONTPOL_LIST; do

    DO_IT="$DO_CONTPOL_IMAGING"

    if [ "${stokes_now}" == "I" ] || [ "${stokes_now}" == "i" ]; then
        if [ "${DO_IT}" == "true" ]; then
            echo "The Stokes-${stokes_now} image is produced in the SelfCal stage, so not running continuum imaging for Stokes-${stokes_now} in beam ${BEAM}"
        fi
        DO_IT=false
    fi

    pol=$(echo "${stokes_now}" | tr '[:upper:]' '[:lower:]')
    . "${PIPELINEDIR}/getContinuumCimagerParams.sh"
    imageCode=restored
    setImageProperties cont

    # Define the image polarisation
    polarisation="${Imager}.Images.image.${imageBase}.polarisation                     = [\"${stokes_now}\"]"
    
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

        setJob science_continuumImage_${stokes_now} cont${stokes_now}
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --nodes=${NUM_NODES_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contImaging-${stokes_now}-%j.out"

${askapsoftModuleCommands}
FAT_NODE_CONT_IMG=${FAT_NODE_CONT_IMG}
#nodeDistribution="--ntasks-per-node=\${NPPN} "
if [ "\${FAT_NODE_CONT_IMG}" == "true" ]; then

    nodelist=\$SLURM_JOB_NODELIST
    
    # Make arrangements to put task 0 (master) of imager on the first node, 
    # and spread the rest out over the other nodes:
    
    newlist=\$(hostname)
    icpu=0
    for proc in \$(seq 1 ${CPUS_PER_CORE_CONT_IMAGING}); do
        for node in \$(scontrol show hostnames \$nodelist); do
            if [[ "\$node" != "\$(hostname)" ]]; then
    	       icpu=\$((icpu+1))
    	       if [[ "\$icpu" -lt "${NUM_CPUS_CONTIMG_SCI}" ]]; then 
    		   newlist=\$newlist,\$node
     	       fi
            fi
        done
    done
    echo "NodeList: "\$nodelist
    echo "NewList: "\$newlist

    #########################
    # The option (using MPICH_RANK_REORDER_METHOD=0) does away with the FAT node 
    # allocation for the master. However, it will allow for cyclic redistribution of the 
    # workers on allocated nodes and will get around the OOM errors. 
    #
    export MPICH_RANK_REORDER_METHOD=0
    nodeDistribution="--nodelist=\$nodelist"
    #
    # Other options that needs exploring -- perhaps with some slurm knobs turned ON? 
    #nodeDistribution="--nodelist=\$newlist"
    #nodeDistribution="--nodelist=\$newlist --distribution=arbitrary"
    #nodeDistribution="--nodelist=\$newlist --distribution=cyclic"
fi

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

# Parameters that can vary with Stokes parameter 
CLEAN_CONTPOL_THRESHOLD_MAJORCYCLE_ARRAY=(${CLEAN_CONTPOL_THRESHOLD_MAJORCYCLE_ARRAY[@]})
CLEAN_CONTPOL_NUM_MAJORCYCLES_ARRAY=(${CLEAN_CONTPOL_NUM_MAJORCYCLES_ARRAY[@]})
CIMAGER_CONTPOL_MINUV_ARRAY=(${CIMAGER_CONTPOL_MINUV_ARRAY[@]})
CIMAGER_CONTPOL_MAXUV_ARRAY=(${CIMAGER_CONTPOL_MAXUV_ARRAY[@]})
CLEAN_CONTPOL_ALGORITHM_ARRAY=(${CLEAN_CONTPOL_ALGORITHM_ARRAY[@]})
CLEAN_CONTPOL_MINORCYCLE_NITER_ARRAY=(${CLEAN_CONTPOL_MINORCYCLE_NITER_ARRAY[@]})
CLEAN_CONTPOL_GAIN_ARRAY=(${CLEAN_CONTPOL_GAIN_ARRAY[@]})
CLEAN_CONTPOL_PSFWIDTH_ARRAY=(${CLEAN_CONTPOL_PSFWIDTH_ARRAY[@]})
CLEAN_CONTPOL_SCALES_ARRAY=(${CLEAN_CONTPOL_SCALES_ARRAY[@]})
CLEAN_CONTPOL_THRESHOLD_MINORCYCLE_ARRAY=(${CLEAN_CONTPOL_THRESHOLD_MINORCYCLE_ARRAY[@]})

LOOP=${SELFCAL_NUM_LOOPS}

cimagerPolImagingParams
dataSelectionPolImage

parset="${parsets}/science_imaging_${stokes_now}_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
cat > "\$parset" <<EOFINNER
${cimagerParams}
${polarisation}
#
\${polParams}
\${dataSelectionParamsPolIm}
#
# Do not apply any calibration
Cimager.calibrate                               = false
EOFINNER

log="${logs}/science_imaging_${stokes_now}_${FIELDBEAM}_\${SLURM_JOB_ID}.log"

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
rejuvenate "${OUTPUT}"/"${msSciAvFull}"
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER
    # 

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
	    DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
	    DEP=$(addDep "$DEP" "$ID_CAL_APPLY_CONT_SCI")
	    ID_CONTPOL_IMG_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	    DEP_CONTPOL_IMG=$(addDep "$DEP_CONTPOL_IMG" "$ID_CONTPOL_IMG_SCI")
	    recordJob "${ID_CONTPOL_IMG_SCI}" "Make final continuum ${stokes_now}-image for beam $BEAM of the science observation, with flags \"$DEP\""
            FLAG_POLIMAGING_DEP=$(addDep "$FLAG_POLIMAGING_DEP" "$ID_CONTPOL_IMG_SCI")
        else
	    echo "Would make the final continuum ${stokes_now}-image for beam $BEAM of the science observation with slurm file $sbatchfile"
        fi

        echo " "

    fi
done
