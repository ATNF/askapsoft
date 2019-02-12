#!/bin/bash -l
#
# Launches a job to build the observation.xml file, and stages data
# ready for ingest into CASDA.
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

if [ "${DO_STAGE_FOR_CASDA}" == "true" ]; then

    if [ "${OTHER_SBIDS}" != "" ]; then
        sbids="sbids                           = ${OTHER_SBIDS}"
    else
        sbids="# No other sbids provided."
    fi

    # If we can't create the output directory
    if [ ! -w "${CASDA_UPLOAD_DIR%/*}" ]; then
        # can't write to the destination - make a new one locally.
        echo "WARNING - desired location for the CASDA output directory ${CASDA_UPLOAD_DIR} is not writeable."
        echo "        - changing output directory to ${OUTPUT}/For-CASDA"
        CASDA_UPLOAD_DIR="${OUTPUT}/For-CASDA"
        mkdir -p ${CASDA_UPLOAD_DIR}
    fi

    CASDA_DIR="${CASDA_UPLOAD_DIR}/${SB_SCIENCE}"

    thumbSizeList="$(echo $THUMBNAIL_SIZE_TEXT | sed -e 's/,/ /g')"



    # Create job that polls for success of casda ingest
    # Only launch this when we are writing the READY file and
    # transitioning the SBs - it will get launched by the casdaupload
    # job itself
    if [ "${WRITE_CASDA_READY}" == "true" ] && [ "${TRANSITION_SB}" == "true" ]; then

        # This slurm job will check for the existence of a DONE file
        # in the CASDA upload directory and, when it appears,
        # transition the SB to COMPLETE.
        # If it hasn't appeared, it relaunches itself to start at some
        # point in the future (given by ${POLLING_DELAY_SEC})
        # If after some longer period of time it hasn't appeared (a
        # length given by ${MAX_POLL_WAIT_TIME}), then it exits with
        # an error.

        if [ "${EMAIL}" != "" ]; then
            emailFail="#SBATCH --mail-type=FAIL
#SBATCH --mail-user=${EMAIL}"
        else
            emailFail="# no email notifications"
        fi
        
        pollingsbatchfile="$slurms/casda_ingest_success_poll.sbatch"
        cat > "$pollingsbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --cluster=zeus
#SBATCH --partition=copyq
#SBATCH --open-mode=append
#SBATCH --output="$slurmOut/slurm-casdaingestPolling-${SB_SCIENCE}.out"
#SBATCH --time=00:05:00
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=casdaPoll${SB_SCIENCE}
#SBATCH --export=NONE
${emailFail}
#SBATCH --requeue   

echo "Polling CASDA deposit success with job \${SLURM_JOB_ID}, \$(date)"

ERROR_FILE=${CASDA_DIR}/POLLING_ERROR

if [ ! -e "${CASDA_DIR}/READY" ]; then
    echo "\$(date): ERROR - READY file not present!" | tee -a \${ERROR_FILE}
    exit 1
fi

READYdate=\$(date +%s -r ${CASDA_DIR}/READY)
NOW=\$(date +%s)
ageOfReady=\$(expr \$NOW - \$READYdate)

module load askapcli

if [ -e "${CASDA_DIR}/ERROR" ]; then
    errmsg=\$(cat ${CASDA_DIR}/ERROR)
    echo "\$(date): ERROR - CASDA ingest failed. SB ${SB_SCIENCE} not transitioned to COMPLETED." | tee -a ${ERROR_FILE}
    if [ "\$(whoami)" == "askapops" ]; then
        schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "ERROR -- CASDA ingest for SB ${SB_SCIENCE} failed with error:\\n\$errmsg" ${SB_SCIENCE}
        annotErr=\$?
        if [ \${annotErr} -ne 0 ]; then
            echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
        fi
    fi
    exit 1
elif [ -e "${CASDA_DIR}/DONE" ]; then
    schedblock transition -s COMPLETED ${SB_SCIENCE}
    err=\$?
    if [ \$err -ne 0 ]; then
        echo "\$(date): ERROR - 'schedblock transition' failed for SB ${SB_SCIENCE} with error code \$err" | tee -a ${ERROR_FILE}
    fi
    if [ "\$(whoami)" == "askapops" ]; then
        if [ \$err -eq 0 ]; then
            schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "Archiving complete. SB ${SB_SCIENCE} transitioned to COMPLETED." ${SB_SCIENCE}
            annotErr=\$?
            if [ \${annotErr} -ne 0 ]; then
                echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
            fi
        else
            schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "ERROR -- Archiving complete but SB ${SB_SCIENCE} failed to transition." ${SB_SCIENCE}
            annotErr=\$?
            if [ \${annotErr} -ne 0 ]; then
                echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
            fi
        fi
    fi
    exit \$err
elif [ "\$ageOfReady" -gt "${MAX_POLL_WAIT_TIME}" ]; then
    echo "\$(date): ERROR - CASDA deposit polling timed out! Waited longer than ${MAX_POLL_WAIT_TIME} sec - SB ${SB_SCIENCE} not transitioned to COMPLETED" | tee -a \${ERROR_FILE}
    if [ "\$(whoami)" == "askapops" ]; then
        schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "ERROR -- CASDA deposit has not completed within ${MAX_POLL_WAIT_TIME} sec - SB ${SB_SCIENCE} not transitioned to COMPLETED" $SB_SCIENCE
        annotErr=\$?
        if [ \${annotErr} -ne 0 ]; then
            echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
        fi
    fi
    exit 1
else
    # Re-submit job with a delay
    sbatch --begin="now+${POLLING_DELAY_SEC}" "${pollingsbatchfile}"
fi

EOFOUTER


    fi
    

    sbatchfile="$slurms/casda_upload.sbatch"
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_CASDA_UPLOAD}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=casdaupload
${exportDirective}
#SBATCH --output="$slurmOut/slurm-casdaupload-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
OUTPUT=${OUTPUT}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

# Define the lists of image names, types, 
PREPARE_FOR_CASDA=true
. ${getArtifacts}

# Set image-related parameters
imageArtifacts=()
imageParams="# Individual image details
#  First the two-dimensional images"
count=0
for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do
    imageArtifacts+=(image\${count})
    imageParams="\${imageParams}
image\${count}.filename  = \${OUTPUT}/\${casdaTwoDimImageNames[i]}
image\${count}.type      = \${casdaTwoDimImageTypes[i]}
image\${count}.project   = ${PROJECT_ID}"
    for size in ${thumbSizeList}; do
        sedstr="s/\.fits/_\${size}.${THUMBNAIL_SUFFIX}/g"
        thumb=\$(echo "\${casdaTwoDimImageNames[i]}" | sed -e \$sedstr)
        if [ -e \${OUTPUT}/"\$thumb" ]; then
            imageParams="\${imageParams}
image\${count}.thumbnail_\${size} = \${OUTPUT}/\${thumb}"
        fi
    done
    ((count++))
done

imageParams="\${imageParams}
#  Next the other images (cubes, spectra)"
for((i=0;i<\${#casdaOtherDimImageNames[@]};i++)); do
    imageArtifacts+=(image\${count})
    imageParams="\${imageParams}
image\${count}.filename  = \${OUTPUT}/\${casdaOtherDimImageNames[i]}
image\${count}.type      = \${casdaOtherDimImageTypes[i]}
image\${count}.project   = ${PROJECT_ID}"
    if [ "\${casdaOtherDimImageMoment0s[i]}" != "" ]; then
        imageParams="\${imageParams}
image\${count}.spectra   = [spec,noise]
image\${count}.spec.filename = \${casdaOtherDimImageSpectra[i]}
image\${count}.spec.type     = spectral_integrated_restored
image\${count}.noise.filename = \${casdaOtherDimImageNoise[i]}
image\${count}.noise.type     = spectral_noise_restored
image\${count}.momentmaps = [mom0,mom1,mom2]
image\${count}.mom0.filename = \${casdaOtherDimImageMoment0s[i]}
image\${count}.mom0.type     = spectral_restored_mom0
image\${count}.mom1.filename = \${casdaOtherDimImageMoment1s[i]}
image\${count}.mom1.type     = spectral_restored_mom1
image\${count}.mom2.filename = \${casdaOtherDimImageMoment2s[i]}
image\${count}.mom2.type     = spectral_restored_mom2
image\${count}.cubelets = [cubelet]
image\${count}.cubelet.filename = \${casdaOtherDimImageCubelets[i]}
image\${count}.cubelet.type     = spectral_restored_3d"
    elif [ "\${casdaOtherDimImageSpectra[i]}" != "" ]; then
        if [ "\${casdaOtherDimImageFDF[i]}" != "" ]; then
            speclist="[spec,noise,FDF,RMSF]"
        else
            speclist="[spec,noise]"
        fi
        imageParams="\${imageParams}
image\${count}.spectra = \${speclist}
image\${count}.spec.filename = \${casdaOtherDimImageSpectra[i]}
image\${count}.spec.type     = cont_restored_\${casdaOtherDimImagePol[i]}
image\${count}.noise.filename = \${casdaOtherDimImageNoise[i]}
image\${count}.noise.type     = cont_noise_\${casdaOtherDimImagePol[i]}"
        if [ "\${casdaOtherDimImageFDF[i]}" != "" ]; then
            imageParams="\${imageParams}
image\${count}.FDF.filename = \${casdaOtherDimImageFDF[i]}
image\${count}.FDF.type     = cont_fdf
image\${count}.RMSF.filename = \${casdaOtherDimImageRMSF[i]}
image\${count}.RMSF.type     = cont_rmsf"
        fi
    fi
    ((count++))
done
imageArtifacts=\$(echo "\${imageArtifacts[@]}" | sed -e 's/ /,/g')

catArtifacts=()
catParams="# Individual catalogue details"
for((i=0;i<\${#catNames[@]};i++)); do
    catArtifacts+=(cat\${i})
    catParams="\${catParams}
cat\${i}.filename = \${OUTPUT}/\${catNames[i]}
cat\${i}.type     = \${catTypes[i]}
cat\${i}.project  = ${PROJECT_ID}"
done
catArtifacts=\$(echo "\${catArtifacts[@]}" | sed -e 's/ /,/g')

msArtifacts=()
msParams="# Individual catalogue details"
for((i=0;i<\${#msNames[@]};i++)); do
    msArtifacts+=(ms\${i})
    msParams="\${msParams}
ms\${i}.filename = \${OUTPUT}/\${msNames[i]}
ms\${i}.project  = ${PROJECT_ID}"
done
msArtifacts=\$(echo "\${msArtifacts[@]}" | sed -e 's/ /,/g')

evalArtifacts=()
evalParams="# Evaluation file details"
for((i=0;i<\${#evalNames[@]};i++)); do
    evalArtifacts+=(eval\${i})
    evalParams="\${evalParams}
eval\${i}.filename = \${OUTPUT}/\${evalNames[i]}
eval\${i}.format   = \${evalFormats[i]}
eval\${i}.project  = ${PROJECT_ID}"
done
evalArtifacts=\$(echo "\${evalArtifacts[@]}" | sed -e 's/ /,/g')

writeREADYfile=${WRITE_CASDA_READY}

parset="${parsets}/casda_upload_\${SLURM_JOB_ID}.in"
log="${logs}/casda_upload_\${SLURM_JOB_ID}.log"
cat > "\$parset" << EOFINNER
# General
outputdir                       = ${CASDA_UPLOAD_DIR}
useAbsolutePaths                = ${CASDA_USE_ABSOLUTE_PATHS}
telescope                       = ASKAP
sbid                            = ${SB_SCIENCE}
${sbids}
obsprogram                      = ${OBS_PROGRAM}
writeREADYfile                  = \${writeREADYfile}

# Images
images.artifactlist             = [\$imageArtifacts]
\${imageParams}

# Source catalogues
catalogues.artifactlist         = [\$catArtifacts]
\${catParams}

# Measurement sets
measurementsets.artifactlist    = [\$msArtifacts]
\${msParams}

# Evaluation reports
evaluation.artifactlist         = [\$evalArtifacts]
\${evalParams}

EOFINNER

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $casdaupload -c "\$parset" > "\$log"
err=\$?
if [ \$err != 0 ]; then
    exit \$err
fi

doTransition=${TRANSITION_SB}

if [ "\${writeREADYfile}" == "true" ] && [ "\${doTransition}" == "true" ]; then

    # We have uploaded successfully and notified CASDA of the data
    # availability. We therefore transition the scheduling block to 
    # PENDINGARCHIVE

    ERROR_FILE=${CASDA_DIR}/SCHEDBLOCK_ERROR

    loadModule askapcli
    schedblock transition -s PENDINGARCHIVE ${SB_SCIENCE}
    err=\$?
    unloadModule askapcli
    if [ \${err} -ne 0 ]; then
        echo "\$(date): ERROR - 'schedblock transition' failed for SB ${SB_SCIENCE} with error code \$err" | tee -a ${ERROR_FILE}
    fi
    if [ "\$(whoami)" == "askapops" ]; then
        if [ \$err -eq 0 ]; then
            schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "Processing complete. SB ${SB_SCIENCE} transitioned to PENDINGARCHIVE." $SB_SCIENCE
            annotErr=\$?
            if [ \${annotErr} -ne 0 ]; then
                echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
            fi
        else
            schedblock annotate -p ${JIRA_ANNOTATION_PROJECT} -c "ERROR -- Processing complete but SB ${SB_SCIENCE} failed to transition." ${SB_SCIENCE}
            annotErr=\$?
            if [ \${annotErr} -ne 0 ]; then
                echo "\$(date): ERROR - 'schedblock annotate' failed with error code \${annotErr}" | tee -a ${ERROR_FILE}
            fi
        fi
    fi

    # Submit job to poll for completion of ingest into the CASDA database
    pollingsbatchfile="$pollingsbatchfile"
    if [ -e "\$pollingsbatchfile" ]; then
        ID_CASDAPOLL=\$(sbatch "\$pollingsbatchfile" | awk '{print \$4}')
        echo "Submitted job to poll for CASDA ingest success, with JobID \$ID_CASDAPOLL"
    fi


fi


EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then    
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:$(echo "${ALL_JOB_IDS}" | sed -e 's/,/:/g')"
        fi
        ID_CASDA=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_CASDA}" "Job to stage data for ingest into CASDA, with flags \"${dep}\""
    else
        echo "Would submit job to stage data for ingest into CASDA, with slurm file $sbatchfile"
    fi


fi
