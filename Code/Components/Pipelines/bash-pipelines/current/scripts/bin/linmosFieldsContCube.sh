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

ID_LINMOS_CONTCUBE_ALL=""

mosaicImageList="restored image residual"

DO_IT=$DO_MOSAIC
if [ "$DO_CONTCUBE_IMAGING" != "true" ]; then
    DO_IT=false
fi

# Don't run if there is only one field
if [ "${NUM_FIELDS}" -eq 1 ]; then
    DO_IT=false
fi

if [ "${DO_MOSAIC_FIELDS}" != "true" ]; then
    DO_IT=false
fi

for imageCode in ${mosaicImageList}; do

    if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
        BEAM=all
        FIELD="."
        tilelistsize=$(echo "${TILE_LIST}" | awk '{print NF}')
        if [ "${tilelistsize}" -gt 1 ]; then
            FULL_TILE_LIST="$TILE_LIST ALL"
        else
            FULL_TILE_LIST="ALL"
        fi
        for TILE in $FULL_TILE_LIST; do
            for POLN in $POL_LIST; do
                pol=$(echo "$POLN" | tr '[:upper:]' '[:lower:]')
                for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
                    setImageProperties contcube
                    if [ -e "${OUTPUT}/${imageName}" ]; then
                        if [ "${DO_IT}" == "true" ]; then
                            echo "Image ${imageName} exists, so not running ${imageCode} continuum mosaicking"
                        fi
                        DO_IT=false
                    fi
                done
            done
        done
    fi

    if [ "${DO_IT}" == "true" ]; then

        sbatchfile=$slurms/linmos_all_contcube_${imageCode}.sbatch
        cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=${NUM_CPUS_CONTCUBE_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONTCUBE_IMAGING}
#SBATCH --job-name=linmosFullCC
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
SB_SCIENCE=${SB_SCIENCE}

FIELD_LIST="$FIELD_LIST"
TILE_LIST="$TILE_LIST"
POL_LIST="${POL_LIST}"
echo "Tile list = \$TILE_LIST"

imageCode=${imageCode}

# If there is only one tile, only include the "ALL" case, which
# mosaics together all fields
if [ "\$(echo \$TILE_LIST | awk '{print NF}')" -gt 1 ]; then
    FULL_TILE_LIST="\$TILE_LIST ALL"
else
    FULL_TILE_LIST="ALL"
fi
echo "Full tile list = \$FULL_TILE_LIST"

for THISTILE in \$FULL_TILE_LIST; do

    echo "Mosaicking tile \$THISTILE"

    # First get the list of FIELDs that contribute to this TILE
    TILE_FIELD_LIST=""
    for FIELD in \$FIELD_LIST; do
        getTile
        if [ "\$THISTILE" == "ALL" ] || [ "\$TILE" == "\$THISTILE" ]; then
            TILE_FIELD_LIST="\$TILE_FIELD_LIST \$FIELD"
        fi
    done
    echo "Tile \$THISTILE has field list \$TILE_FIELD_LIST"

    for POLN in \$POL_LIST; do
    
        pol=\$(echo "\$POLN" | tr '[:upper:]' '[:lower:]')
    
        for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
    
            imList=""
            wtList=""
            BEAM=all
            for FIELD in \${TILE_FIELD_LIST}; do
                setImageProperties contcube
                if [ -e "\${FIELD}/\${imageName}" ]; then
                    if [ "\${imList}" == "" ]; then
                        imList="\${FIELD}/\${imageName}"
                        wtList="\${FIELD}/\${weightsImage}"
                    else
                        imList="\${imList},\${FIELD}/\${imageName}"
                        wtList="\${wtList},\${FIELD}/\${weightsImage}"
                    fi
                fi
            done

            if [ "\$THISTILE" == "ALL" ]; then
                jobCode=linmosCC_Full_\${imageCode}\${subband}
            else
                jobCode=linmosCC_\${THISTILE}_\${imageCode}\${subband}
            fi

            if [ "\${imList}" != "" ]; then
                FIELD="."
                TILE=\$THISTILE
                setImageProperties contcube
                echo "Mosaicking to form \${imageName}"
                parset=${parsets}/science_\${jobCode}_\${SLURM_JOB_ID}.in
                log=${logs}/science_\${jobCode}_\${SLURM_JOB_ID}.log
                cat > "\${parset}" << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.outname          = \$imageName
linmos.outweight        = \$weightsImage
linmos.weighttype       = FromWeightImages
EOFINNER

                NCORES=${NUM_CPUS_CONTCUBE_SCI}
                NPPN=${CPUS_PER_CORE_CONTCUBE_IMAGING}
                aprun -n \${NCORES} -N \${NPPN} $linmosMPI -c "\$parset" > "\$log"
                err=\$?
                for im in \$(echo "\${imList}" | sed -e 's/,/ /g'); do
                    rejuvenate "\$im"
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
done
EOFOUTER

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            FULL_LINMOS_CONTCUBE_DEP=$(echo "${FULL_LINMOS_CONTCUBE_DEP}" | sed -e 's/afterok/afterany/g')
	    ID_LINMOS_CONTCUBE_ALL=$(sbatch ${FULL_LINMOS_CONTCUBE_DEP} "$sbatchfile" | awk '{print $4}')
	    recordJob "${ID_LINMOS_CONTCUBE_ALL}" "Make a mosaic ${imageCode} continuum cube of the science observation, with flags \"${FULL_LINMOS_CONTCUBE_DEP}\""
        else
	    echo "Would make a mosaic ${imageCode} continuum cube of the science observation, with slurm file $sbatchfile"
        fi

    fi

done

echo " "

