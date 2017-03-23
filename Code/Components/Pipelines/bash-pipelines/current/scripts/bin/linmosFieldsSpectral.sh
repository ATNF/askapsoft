#!/bin/bash -l
#
# Launches a job to mosaic all individual beam continuum images to a
# single image. After completion, runs the source-finder on the
# mosaicked image.
#
# @copyright (c) 2016 CSIRO
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

ID_LINMOS_SPECTRAL_ALL=""

mosaicImageList="restored contsub image residual"

for imageCode in ${mosaicImageList}; do 

    for subband in ${SUBBAND_WRITER_LIST}; do

        DO_IT=$DO_MOSAIC
        if [ "$DO_SPECTRAL_IMAGING" != "true" ]; then
            DO_IT=false
        fi

        # Don't run if there is only one field
        if [ ${NUM_FIELDS} -eq 1 ]; then
            DO_IT=false
        fi

        if [ "${DO_MOSAIC_FIELDS}" != "true" ]; then
            DO_IT=false
        fi

        if [ "${DO_IT}" == "true" ] && [ "${CLOBBER}" != "true" ]; then
            FIELD="."
            BEAM=all
            if [ `echo $TILE_LIST | awk '{print NF}'` -gt 1 ]; then
                FULL_TILE_LIST="$TILE_LIST ALL"
            else
                FULL_TILE_LIST="ALL"
            fi
            for TILE in $FULL_TILE_LIST; do
                setImageProperties spectral
                if [ -e ${OUTPUT}/${imageName} ]; then
                    if [ $DO_IT == true ]; then
                        echo "Image ${imageName} exists, so not running its spectral-line mosaicking"
                    fi
                    DO_IT=false
                fi
            done
        fi

        if [ $DO_IT == true ]; then

            code=${imageCode}
            if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ]; then
                code="${code}${subband}"
            fi
            sbatchfile=$slurms/linmos_all_spectral_${code}.sbatch
            cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=${NUM_CPUS_SPECTRAL_LINMOS}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SPEC_IMAGING}
#SBATCH --job-name=linmosFullS${imageCode}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-linmosS-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

IMAGE_BASE_SPECTRAL=${IMAGE_BASE_SPECTRAL}
SB_SCIENCE=${SB_SCIENCE}

FIELD_LIST="$FIELD_LIST"
TILE_LIST="$TILE_LIST"
echo "Tile list = \$TILE_LIST"

# If there is only one tile, only include the "ALL" case, which
# mosaics together all fields
if [ \`echo \$TILE_LIST | awk '{print NF}'\` -gt 1 ]; then
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
        if [ \$THISTILE == "ALL" ] || [ \$TILE == \$THISTILE ]; then
            TILE_FIELD_LIST="\$TILE_FIELD_LIST \$FIELD"
        fi
    done
    echo "Tile \$THISTILE has field list \$TILE_FIELD_LIST"

    imageCode=${imageCode}
    DO_ALT_IMAGER=${DO_ALT_IMAGER}
    NUM_SPECTRAL_CUBES=${NUM_SPECTRAL_CUBES}
    subband="${subband}"
    imList=""       
    wtList=""
    BEAM=all
    for FIELD in \${TILE_FIELD_LIST}; do
        setImageProperties spectral
        if [ "\${imageCode}" != "restored" ]; then
            weightsImage="\${weightsImage}.\${imageCode}"
        fi
        if [ -e \${FIELD}/\${imageName} ]; then
            if [ "\${imList}" == "" ]; then
                imList="\${FIELD}/\${imageName}"
                wtList="\${FIELD}/\${weightsImage}"
            else
                imList="\${imList},\${FIELD}/\${imageName}"
                wtList="\${wtList},\${FIELD}/\${weightsImage}"
            fi
        fi
    done

    if [ \$THISTILE == "ALL" ]; then
        jobCode=linmosS_Full_\${imageCode}
    else
        jobCode=linmosS_\${THISTILE}_\${imageCode}
    fi
    if [ "\${NUM_SPECTRAL_CUBES}" -gt 1 ]; then
        jobCode="\${jobCode}\${subband}"
    fi

    if [ "\${imList}" != "" ]; then
        FIELD="."
        TILE=\$THISTILE
        setImageProperties spectral
        echo "Mosaicking to form \${imageName}"
        parset=${parsets}/science_\${jobCode}_\${SLURM_JOB_ID}.in
        log=${logs}/science_\${jobCode}_\${SLURM_JOB_ID}.log
        cat > \${parset} << EOFINNER
linmos.names            = [\${imList}]
linmos.weights          = [\${wtList}]
linmos.outname          = \$imageName
linmos.outweight        = \$weightsImage
linmos.weighttype       = FromWeightImages
EOFINNER

        NCORES=${NUM_CPUS_SPECTRAL_LINMOS}
        NPPN=${CPUS_PER_CORE_SPEC_IMAGING}
        aprun -n \${NCORES} -N \${NPPN} $linmosMPI -c \$parset > \$log
        err=\$?
        for im in \`echo \${imList} | sed -e 's/,/ /g'\`; do
            rejuvenate \$im
        done
        extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} \${jobCode} "txt,csv"
        if [ \$err != 0 ]; then
            exit \$err
        fi
    else
        echo "WARNING - no good images were found for mosaicking image type '\${imageCode}'!"
    fi
done
EOFOUTER

            if [ $SUBMIT_JOBS == true ]; then
                FULL_LINMOS_SPECTRAL_DEP=`echo $FULL_LINMOS_SPECTRAL_DEP | sed -e 's/afterok/afterany/g'`
	        ID_LINMOS_SPECTRAL_ALL=`sbatch $FULL_LINMOS_SPECTRAL_DEP $sbatchfile | awk '{print $4}'`
                if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ];then
	            recordJob ${ID_LINMOS_SPECTRAL_ALL} "Make a mosaic ${imageCode} (subband ${subband}) spectral cube of the science observation, with flags \"${FULL_LINMOS_SPECTRAL_DEP}\""
                else
                    recordJob ${ID_LINMOS_SPECTRAL_ALL} "Make a mosaic ${imageCode} spectral cube of the science observation, with flags \"${FULL_LINMOS_SPECTRAL_DEP}\""
                fi
            else
                if [ "${NUM_SPECTRAL_CUBES}" -gt 1 ];then
	            echo "Would make a mosaic ${imageCode} (subband ${subband}) spectral cube of the science observation, with slurm file $sbatchfile"
                else
                    echo "Would make a mosaic ${imageCode} spectral cube of the science observation, with slurm file $sbatchfile"
                fi
            fi
            
            echo " "

        fi

    done

done
