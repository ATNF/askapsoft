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

ID_LINMOS_CONTCUBE=""

mosaicImageList="restored image residual"

DO_IT=$DO_MOSAIC
if [ "$DO_CONTCUBE_IMAGING" != "true" ]; then
    DO_IT=false
fi

for imageCode in ${mosaicImageList}; do

    if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
        BEAM=all
        for POLN in $POL_LIST; do
            pol=$(echo "$POLN" | tr '[:upper:]' '[:lower:]')
            for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
                setImageProperties contcube
                if [ -e "${OUTPUT}/${imageName}" ]; then
                    if [ "${DO_IT}" == "true" ]; then
                        echo "Image ${imageName} exists, so not running ${imageCode} continuum cube mosaicking"
                    fi
                    DO_IT=false
                fi
            done
        done
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


        setJob "linmosContCube${imageCode}" "linmosCC${imageCode}"
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=${NUM_CPUS_CONTCUBE_LINMOS}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTCUBE_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-linmosCC-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

DO_ALT_IMAGER_CONTCUBE="${DO_ALT_IMAGER_CONTCUBE}"
ALT_IMAGER_SINGLE_FILE_CONTCUBE="${ALT_IMAGER_SINGLE_FILE_CONTCUBE}"
IMAGE_BASE_CONTCUBE=${IMAGE_BASE_CONTCUBE}
FIELD=${FIELD}
POL_LIST="${POL_LIST}"
BEAMS_TO_USE="${BEAMS_TO_USE}"
imageCode=${imageCode}
IMAGETYPE_CONTCUBE="${IMAGETYPE_CONTCUBE}"

for POLN in \$POL_LIST; do

    pol=\$(echo "\$POLN" | tr '[:upper:]' '[:lower:]')

    for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
    
        imList=""
        wtList=""
        for BEAM in \${BEAMS_TO_USE}; do
            setImageProperties contcube
            im="\${imageName}"
            wt="\${weightsImage}"
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
    
        jobCode=${jobname}_\${imageCode}\${subband}
    
        if [ "\${imList}" != "" ]; then
            BEAM=all
            setImageProperties contcube
            if [ "\${imageCode}" != "restored" ]; then
                weightsImage="\${weightsImage%%.fits}.\${imageCode}"
                if [ "\${IMAGETYPE_CONTCUBE}" == "fits" ]; then
                    weightsImage="\${weightsImage}.fits"
                fi
            fi
            echo "Mosaicking \${imList} to form \${imageName}"
            parset="${parsets}/science_\${jobCode}_\${pol}_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
            log="${logs}/science_\${jobCode}_\${pol}_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
            cat > "\${parset}" << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.imagetype        = \${IMAGETYPE_CONTCUBE}
linmos.outname          = \${imageName%%.fits}
linmos.outweight        = \${weightsImage%%.fits}
linmos.weighttype       = ${LINMOS_SINGLE_FIELD_WEIGHTTYPE}
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

            NCORES=${NUM_CPUS_CONTCUBE_LINMOS}
            NPPN=${CPUS_PER_CORE_CONTCUBE_IMAGING}
            srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $linmosMPI -c "\$parset" > "\$log"
            err=\$?
            for im in \$(echo "\${imList}" | sed -e 's/,/ /g'); do
                rejuvenate "\${im}"
            done
            extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} \${jobCode} "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            fi

            if [ "\${imageCode}" == "restored" ] || [ "\${imageCode}" == "residual" ]; then
                # Find the cube statistics
                loadModule mpi4py
                echo "Finding cube stats for \${imageName}"
                srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${imageName}
            fi

        else
            echo "WARNING - no good images were found for mosaicking image type '\${imageCode}'!"
        fi
    done
done
EOFOUTER

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            DEP_CONTCUBE=$(echo "$DEP_CONTCUBE" | sed -e 's/afterok/afterany/g')
	    ID_LINMOS_CONTCUBE=$(sbatch ${DEP_CONTCUBE} "$sbatchfile" | awk '{print $4}')
            if [ "${imageCode}" == "restored" ]; then
                ID_LINMOS_CONTCUBE_RESTORED=${ID_LINMOS_CONTCUBE}
            fi
	    recordJob "${ID_LINMOS_CONTCUBE}" "Make a mosaic ${imageCode} continuum cube of the science observation, field $FIELD, with flags \"${DEP_CONTCUBE}\""
            FULL_LINMOS_CONTCUBE_DEP=$(addDep "${FULL_LINMOS_CONTCUBE_DEP}" "${ID_LINMOS_CONTCUBE}")
        else
	    echo "Would make a mosaic ${imageCode} continuum cube of the science observation, field $FIELD, with slurm file $sbatchfile"
        fi


    fi
done
echo " "
