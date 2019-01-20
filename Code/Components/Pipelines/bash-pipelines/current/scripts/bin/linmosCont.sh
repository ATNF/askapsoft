#!/bin/bash -l
#
# Launches a job to mosaic all individual beam continuum images to a
# single image. After completion, runs the source-finder on the
# mosaicked image.
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

ID_LINMOS_CONT=""

mosaicImageList="restored altrestored image residual"

NLOOPS=0
if [ "${DO_SELFCAL}" == "true" ] && [ "${MOSAIC_SELFCAL_LOOPS}" == "true" ]; then
    NLOOPS=$SELFCAL_NUM_LOOPS
fi
BEAM=all
for((LOOP=0;LOOP<=NLOOPS;LOOP++)); do
    for imageCode in ${mosaicImageList}; do
        for((TTERM=0;TTERM<NUM_TAYLOR_TERMS;TTERM++)); do

            DO_IT=$DO_MOSAIC
            if [ "$DO_CONT_IMAGING" != "true" ]; then
                DO_IT=false
            fi

            if [ $NLOOPS -gt 0 ]; then
                tag="${imageCode}T${TTERM}L${LOOP}"
            else
                tag="${imageCode}T${TTERM}"
            fi
            
            if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
                setImageProperties cont
                if [ -e "${OUTPUT}/${imageName}" ]; then
                    if [ "${DO_IT}" == "true" ]; then
                        echo "Image ${imageName} exists, so not running continuum mosaicking"
                    fi
                    DO_IT=false
                fi
            fi

            # Don't bother looking for altrestored images if we aren't doing that mode
            if [ "${imageCode}" == "altrestored" ] && [ "${RESTORE_PRECONDITIONER_LIST}" == "" ]; then
                DO_IT=false
            fi

            if [ "${DO_IT}" == "true" ]; then

                if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ] && [ "$DIRECTION_SCI" == "" ]; then
                    reference="# No reference image or offsets, as we take the image centres"
                else
                    reference="# Reference image for offsets
linmos.feeds.centreref  = 0      
linmos.feeds.spacing    = ${LINMOS_BEAM_SPACING}
# Beam offsets
${LINMOS_BEAM_OFFSETS}"
                fi


                setJob linmosCont_${tag} linmosC${tag}
                cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-linmosC-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

DO_ALT_IMAGER_CONT="${DO_ALT_IMAGER_CONT}"
NUM_TAYLOR_TERMS=${NUM_TAYLOR_TERMS}
IMAGE_BASE_CONT=${IMAGE_BASE_CONT}
FIELD=${FIELD}
BEAMS_TO_USE="${BEAMS_TO_USE}"
IMAGETYPE_CONT="${IMAGETYPE_CONT}"

LOOP=$LOOP
imageCode=$imageCode
TTERM=$TTERM

imList=""
wtList=""
for BEAM in \${BEAMS_TO_USE}; do
    setImageProperties cont
    if [ "\$LOOP" -eq 0 ]; then
        DIR="."
    else
        DIR="selfCal_\${imageBase}/Loop\${LOOP}"
    fi
    im="\${DIR}/\${imageName}"
    wt="\${DIR}/\${weightsImage}"
    if [ -e "\${im}" ]; then
        if [ "\${imList}" == "" ]; then
            imList="\${im%%.fits}"
            wtList="\${wt%%.fits}"
        else
            imList="\${imList},\${im%%.fits}"
            wtList="\${wtList},\${wt%%.fits}"
        fi
    fi
done

jobCode=${jobname}_\${imageCode}
if [ "\${NUM_TAYLOR_TERMS}" -gt 1 ]; then
    jobCode=\${jobCode}_T\${TTERM}
fi
if [ "\$LOOP" -gt 0 ]; then
    jobCode=\${jobCode}_L\${LOOP}
fi

if [ "\${imList}" != "" ]; then
    BEAM=all
    setImageProperties cont
    if [ "\$LOOP" -gt 0 ]; then
        imageName="\${imageName%%.fits}.SelfCalLoop\${LOOP}"
        weightsImage="\${weightsImage%%.fits}.SelfCalLoop\${LOOP}"
        if [ "\${IMAGETYPE_CONT}" == "fits" ]; then
            imageName="\${imageName}.fits"
            weightsImage="\${weightsImage}.fits"
        fi
    fi
    echo "Mosaicking to form \${imageName} using weighttype=${LINMOS_SINGLE_FIELD_WEIGHTTYPE}"
    echo "Image list = \${imList}"
    parset="${parsets}/science_\${jobCode}_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
    log="${logs}/science_\${jobCode}_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    cat > "\${parset}" << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.imagetype        = \${IMAGETYPE_CONT}
linmos.outname          = \${imageName%%.fits}
linmos.outweight        = \${weightsImage%%.fits}
linmos.weighttype       = ${LINMOS_SINGLE_FIELD_WEIGHTTYPE}
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $linmosMPI -c "\$parset" > "\$log"
    err=\$?
    for im in \$(echo "\${imList}" | sed -e 's/,/ /g'); do
        rejuvenate "\${im}"
    done
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} \${jobCode} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
else
    echo "WARNING - no good images were found for mosaicking image type '\${imageCode}'!"
fi
EOFOUTER

                if [ "${SUBMIT_JOBS}" == "true" ]; then
                    FLAG_IMAGING_DEP=$(echo "${FLAG_IMAGING_DEP}" | sed -e 's/afterok/afterany/g')
	            ID_LINMOS_CONT=$(sbatch ${FLAG_IMAGING_DEP} "$sbatchfile" | awk '{print $4}')
                    if [ "${imageCode}" == "restored" ]; then
                        ID_LINMOS_CONT_RESTORED=${ID_LINMOS_CONT}
                    fi
	            recordJob "${ID_LINMOS_CONT}" "Make a mosaic continuum image of the science observation, field $FIELD, tag $tag, with flags \"${FLAG_IMAGING_DEP}\""
                    FULL_LINMOS_CONT_DEP=$(addDep "${FULL_LINMOS_CONT_DEP}" "${ID_LINMOS_CONT}")
                else
	            echo "Would make a mosaic image of the science observation, field $FIELD, tag $tag, with slurm file $sbatchfile"
                fi

                echo " "

            fi
            
        done
        unset TTERM
    done
done
unset LOOP


