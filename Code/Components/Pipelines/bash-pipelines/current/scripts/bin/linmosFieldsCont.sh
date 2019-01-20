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

ID_LINMOS_CONT_ALL=""

mosaicImageList="restored altrestored image residual"

BEAM=all
FIELD="."
tilelistsize=$(echo "${TILE_LIST}" | awk '{print NF}')
if [ "${tilelistsize}" -gt 1 ]; then
    FULL_TILE_LIST="$TILE_LIST ALL"
else
    FULL_TILE_LIST="ALL"
fi
for TILE in $FULL_TILE_LIST; do
    for imageCode in ${mosaicImageList}; do
        for((TTERM=0;TTERM<NUM_TAYLOR_TERMS;TTERM++)); do

            DO_IT=$DO_MOSAIC
            if [ "$DO_CONT_IMAGING" != "true" ]; then
                DO_IT=false
            fi

            if [ "${DO_MOSAIC_FIELDS}" != "true" ]; then
                DO_IT=false
            fi

            tag="${imageCode}T${TTERM}${TILE}"

            if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
                setImageProperties cont
                if [ -e "${OUTPUT}/${imageName}" ]; then
                    if [ "${DO_IT}" == "true" ]; then
                        echo "Image ${imageName} exists, so not running continuum mosaicking"
                    fi
                    DO_IT=false
                fi
            fi

            if [ "${DO_IT}" == "true" ]; then

                sbatchfile="$slurms/linmos_all_cont.sbatch"
                cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=linmosFullC
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
SB_SCIENCE=${SB_SCIENCE}

FIELD_LIST="$FIELD_LIST"
TILE_LIST="$TILE_LIST"

IMAGETYPE_CONT="${IMAGETYPE_CONT}"

# If there is only one tile, only include the "ALL" case, which
# mosaics together all fields
if [ "\$(echo \$TILE_LIST | awk '{print NF}')" -gt 1 ]; then
    FULL_TILE_LIST="\$TILE_LIST ALL"
else
    FULL_TILE_LIST="ALL"
fi

THISTILE=$TILE
imageCode=$imageCode
TTERM=$TTERM

echo "Mosaicking tile \$THISTILE"

# First get the list of FIELDs that contribute to this TILE
TILE_FIELD_LIST=""
IFS="${IFS_FIELDS}"
for FIELD in \$FIELD_LIST; do
    getTile
    if [ "\$THISTILE" == "ALL" ] || [ "\$TILE" == "\$THISTILE" ]; then
        if [ "\${TILE_FIELD_LIST}" == "" ]; then
            TILE_FIELD_LIST="\$FIELD"
        else
            TILE_FIELD_LIST="\$TILE_FIELD_LIST
\$FIELD"
        fi
    fi
done
echo "Tile \$THISTILE has field list \$TILE_FIELD_LIST"

imList=""
wtList=""
BEAM=all
listCount=0
for FIELD in \${TILE_FIELD_LIST}; do
    setImageProperties cont
    im="\${FIELD}/\${imageName}"
    wt="\${FIELD}/\${weightsImage}"
    if [ -e "\${im}" ]; then
        ((listCount++))
        if [ "\${imList}" == "" ]; then
            imList="\${im%%.fits}"
            wtList="\${wt%%.fits}"
        else
            imList="\${imList},\${im%%.fits}"
            wtList="\${wtList},\${wt%%.fits}"
        fi
    fi
done
IFS="${IFS_DEFAULT}"

if [ "\$THISTILE" == "ALL" ]; then
    jobCode=linmosC_Full_\${imageCode}
else
    jobCode=linmosC_\${THISTILE}_\${imageCode}
fi
if [ "\${NUM_TAYLOR_TERMS}" -gt 1 ]; then
    jobCode=\${jobCode}_T\${TTERM}
fi

if [ "\${imList}" != "" ]; then
    FIELD="."
    TILE=\$THISTILE
    setImageProperties cont

    if [ \${listCount} -gt 1 ]; then
        echo "Mosaicking to form \${imageName}"
        parset="${parsets}/science_\${jobCode}_\${SLURM_JOB_ID}.in"
        log="${logs}/science_\${jobCode}_\${SLURM_JOB_ID}.log"
        cat > "\${parset}" << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.imagetype        = \${IMAGETYPE_CONT}
linmos.outname          = \${imageName%%.fits}
linmos.outweight        = \${weightsImage%%.fits}
linmos.weighttype       = FromWeightImages
EOFINNER

        NCORES=1
        NPPN=1
        srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} $linmosMPI -c "\$parset" > "\$log"
        err=\$?
        for im in \$(echo "\${imList}" | sed -e 's/,/ /g'); do
            rejuvenate "\$im"
        done
        extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} \${jobCode} "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        fi
    else
        # imList and wtList just have a single image -
        #  just do a simple copy rather than running linmos
        if [ "\${IMAGETYPE_CONT}" == "fits" ]; then
            imList="\${imList}.fits"
            wtList="\${wtList}.fits"
        fi
        echo "Copying \${imList} to form \${imageName}"
        cp -r \${imList} \${imageName}
        cp -r \${wtList} \${weightsImage}
    fi

else
    echo "WARNING - no good images were found for mosaicking image type '\${imageCode}'!"
fi
EOFOUTER

                if [ "${SUBMIT_JOBS}" == "true" ]; then
                    FULL_LINMOS_CONT_DEP=$(echo "${FULL_LINMOS_CONT_DEP}" | sed -e 's/afterok/afterany/g')
	            ID_LINMOS_CONT_ALL=$(sbatch ${FULL_LINMOS_CONT_DEP} "$sbatchfile" | awk '{print $4}')
                    if [ "${imageCode}" == "restored" ]; then
                        ID_LINMOS_CONT_ALL_RESTORED=${ID_LINMOS_CONT_ALL}
                    fi
	            recordJob "${ID_LINMOS_CONT_ALL}" "Make a mosaic continuum image of the science observation, tag $tag, with flags \"${FULL_LINMOS_CONT_DEP}\""
                else
	            echo "Would make a mosaic image of the science observation, tag $tag, with slurm file $sbatchfile"
                fi

                echo " "

            fi

        done
        unset TTERM
    done
done
