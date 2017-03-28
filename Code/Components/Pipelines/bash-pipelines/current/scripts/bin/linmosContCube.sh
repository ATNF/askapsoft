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


        setJob "linmosContCube${code}" "linmosCC${code}"
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=${NUM_CPUS_CONTCUBE_LINMOS}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTCUBE_IMAGING}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-linmosCC-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

DO_ALT_IMAGER_CONTCUBE="${DO_ALT_IMAGER_CONTCUBE}"
IMAGE_BASE_CONTCUBE=${IMAGE_BASE_CONTCUBE}
FIELD=${FIELD}
POL_LIST="${POL_LIST}"
BEAMS_TO_USE="${BEAMS_TO_USE}"
imageCode=${imageCode}

for POLN in \$POL_LIST; do

    pol=\$(echo "\$POLN" | tr '[:upper:]' '[:lower:]')

    for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
    
        beamList=""
        for BEAM in \${BEAMS_TO_USE}; do
            setImageProperties contcube
            if [ -e "\${imageName}" ]; then
                if [ "\${beamList}" == "" ]; then
                    beamList="\${imageName}"
                else
                    beamList="\${beamList},\${imageName}"
                fi
            fi
        done
    
        jobCode=${jobname}_\${imageCode}\${subband}
    
        if [ "\${beamList}" != "" ]; then
            BEAM=all
            setImageProperties contcube
            echo "Mosaicking \${beamList} to form \${imageName}"
            parset=${parsets}/science_\${jobCode}_\${pol}_${FIELDBEAM}_\${SLURM_JOB_ID}.in
            log=${logs}/science_\${jobCode}_\${pol}_${FIELDBEAM}_\${SLURM_JOB_ID}.log
            cat > "\${parset}" << EOFINNER
linmos.names            = [\${beamList}]
linmos.outname          = \$imageName
linmos.outweight        = \$weightsImage
linmos.weighttype       = FromPrimaryBeamModel
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

            NCORES=${NUM_CPUS_CONTCUBE_LINMOS}
            NPPN=${CPUS_PER_CORE_CONTCUBE_IMAGING}
            aprun -n \${NCORES} -N \${NPPN} $linmosMPI -c "\$parset" > "\$log"
            err=\$?
            for im in \$(echo "\${beamList}" | sed -e 's/,/ /g'); do
                rejuvenate "\${im}"
            done
            extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} \${jobCode} "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
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
	    recordJob "${ID_LINMOS_CONTCUBE}" "Make a mosaic ${imageCode} continuum cube of the science observation, field $FIELD, with flags \"${DEP_CONTCUBE}\""
            FULL_LINMOS_CONTCUBE_DEP=$(addDep "${FULL_LINMOS_CONTCUBE_DEP}" "${ID_LINMOS_CONTCUBE}")
        else
	    echo "Would make a mosaic ${imageCode} continuum cube of the science observation, field $FIELD, with slurm file $sbatchfile"
        fi


    fi
done
echo " "
