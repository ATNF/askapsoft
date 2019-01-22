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

ID_LINMOS_SPECTRAL=""

mosaicImageList="restored contsub image residual"

if [ "${IMAGE_AT_BEAM_CENTRES}" == "true" ] && [ "$DIRECTION_SCI" == "" ]; then
    reference="# No reference image or offsets, as we take the image centres"
else
    reference="# Reference image for offsets
linmos.feeds.centreref  = 0
linmos.feeds.spacing    = ${LINMOS_BEAM_SPACING}
# Beam offsets
${LINMOS_BEAM_OFFSETS}"
fi

for imageCode in ${mosaicImageList}; do

    for subband in ${SUBBAND_WRITER_LIST}; do

        DO_IT=$DO_MOSAIC
        if [ "${DO_SPECTRAL_IMAGING}" != "true" ]; then
            DO_IT=false
        fi
        
        if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
            BEAM=all
            setImageProperties spectral
            if [ -e "${OUTPUT}/${imageName}" ]; then
                if [ "${DO_IT}" == "true" ]; then
                    echo "Image ${imageName} exists, so not running its spectral-line mosaicking"
                fi
                DO_IT=false
            fi
        fi

        if [ "${DO_IT}" == "true" ]; then

            code=${imageCode}
            if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ]; then
                code="${code}${subband}"
            fi
            setJob "linmosSpectral_${code}" "linmosS${code}"
            cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
## #SBATCH --mem=62gb
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=${NUM_CPUS_SPECTRAL_LINMOS}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SPEC_IMAGING}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-linmosS-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

IMAGE_BASE_SPECTRAL=${IMAGE_BASE_SPECTRAL}
FIELD=${FIELD}
BEAMS_TO_USE="${BEAMS_TO_USE}"

imageCode=${imageCode}
DO_ALT_IMAGER_SPECTRAL="${DO_ALT_IMAGER_SPECTRAL}"
ALT_IMAGER_SINGLE_FILE="${ALT_IMAGER_SINGLE_FILE}"
NUM_SPECTRAL_CUBES=${NUM_SPECTRAL_CUBES}
subband="${subband}"
IMAGETYPE_SPECTRAL="${IMAGETYPE_SPECTRAL}"

imList=""
wtList=""
for BEAM in \${BEAMS_TO_USE}; do
    setImageProperties spectral
    im="\${imageName}"
    wt="\${weightsImage}"
    if [ -e "\${im}" ]; then
        echo "   Found \${im} - Adding to imList"
        if [ "\${imList}" == "" ]; then
            imList="\${im%%.fits}"
            wtList="\${wt%%.fits}"
        else
            imList="\${imList},\${im%%.fits}"
            wtList="\${wtList},\${wt%%.fits}"
        fi
    else
        echo "   \${im} not found!"
    fi
done

jobCode=${jobname}_\${imageCode}
if [ "\${NUM_SPECTRAL_CUBES}" -gt 1 ]; then
    jobCode="\${jobCode}\${subband}"
fi

if [ "\${imList}" != "" ]; then
    echo "Have list of images to be mosaicked:  \$imList"
    BEAM=all
    setImageProperties spectral
    if [ "\${imageCode}" != "restored" ]; then
        weightsImage="\${weightsImage%%.fits}.\${imageCode}"
        if [ "\${IMAGETYPE_SPECTRAL}" == "fits" ]; then
            weightsImage="\${weightsImage}.fits"
        fi
    fi
    echo "Mosaicking \${imList} to form \${imageName}"
    parset="${parsets}/science_\${jobCode}_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
    log="${logs}/science_\${jobCode}_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    cat > "\${parset}" << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.imagetype        = \${IMAGETYPE_SPECTRAL}
linmos.outname          = \${imageName%%.fits}
linmos.outweight        = \${weightsImage%%.fits}
linmos.weighttype       = ${LINMOS_SINGLE_FIELD_WEIGHTTYPE}
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

    NCORES=${NUM_CPUS_SPECTRAL_LINMOS}
    NPPN=${CPUS_PER_CORE_SPEC_IMAGING}
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $linmosMPI -c "\$parset" > "\$log"
    err=\$?
    for im in \$(echo "\${imList}" | sed -e 's/,/ /g'); do
        rejuvenate "\${im}"
    done
    extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} \${jobCode} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

    if [ "\${imageCode}" == "restored" ] || [ "\${imageCode}" == "residual" ] || [ "\${imageCode}" == "contsub" ]; then
        # Find the cube statistics
        loadModule mpi4py
        cube=${imageName}
        echo "Finding cube stats for \${cube}"
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${cube}
    fi

else
    echo "WARNING - no good images were found for mosaicking image type '\${imageCode}'!"
fi
EOFOUTER

            if [ "${SUBMIT_JOBS}" == "true" ]; then
                DEP=$(echo "${DEP_SPECIMG}" | sed -e 's/afterok/afterany/g')
                if [ "${imageCode}" == "contsub" ]; then
                    DEP=$(addDep "${DEP}" "$(echo "${DEP_SPECIMCONTSUB}" | sed -e 's/-d afterok://g')")
                fi
	        ID_LINMOS_SPECTRAL=$(sbatch ${DEP} "$sbatchfile" | awk '{print $4}')
                if [ "${imageCode}" == "restored" ]; then
                    DEP_LINMOS_SPECTRAL_RESTORED=$(addDep "${DEP_LINMOS_SPECTRAL_RESTORED}" "${ID_LINMOS_SPECTRAL}")
                elif [ "${imageCode}" == "contsub" ]; then
                    DEP_LINMOS_SPECTRAL_CONTSUB=$(addDep "${DEP_LINMOS_SPECTRAL_CONTSUB}" "${ID_LINMOS_SPECTRAL}")
                fi
                
                if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ];then
	            recordJob "${ID_LINMOS_SPECTRAL}" "Make a mosaic ${imageCode} (subband ${subband}) spectral cube of the science observation, field $FIELD, with flags \"${DEP}\""
                else
                    recordJob "${ID_LINMOS_SPECTRAL}" "Make a mosaic ${imageCode} spectral cube of the science observation, field $FIELD, with flags \"${DEP}\""
                fi
                FULL_LINMOS_SPECTRAL_DEP=$(addDep "${FULL_LINMOS_SPECTRAL_DEP}" "${ID_LINMOS_SPECTRAL}")
            else
                if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ];then
	            echo "Would make a mosaic ${imageCode} (subband ${subband}) spectral cube of the science observation, field $FIELD with slurm file $sbatchfile"
                else
                    echo "Would make a mosaic ${imageCode} spectral cube of the science observation, field $FIELD with slurm file $sbatchfile"
                fi
            fi
            
        fi

    done

done

echo " "

