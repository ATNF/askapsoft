#!/bin/bash -l
#
# Launches a job to mosaic all individual beam images to a single
# image. After completion, runs the source-finder on the mosaicked
# image.
#
# @copyright (c) 2015 CSIRO
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

ID_LINMOS_SCI=""

DO_IT=$DO_MOSAIC

BEAM=all
setImageBase spectral
getAltPrefix

for subband in ${wrList}; do

    mosImage=image.${subband}.${imageBase}

    if [ $NUM_TAYLOR_TERMS -gt 1 ]; then
        mosImage="${mosImage}.taylor.0"
    fi

    mosImage="${mosImage}.restored"

    if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${mosImage} ]; then
        if [ $DO_IT == true ]; then
            echo "Image ${mosImage} exists, so not running continuum mosaicking"
        fi
        DO_IT=false
    fi

    if [ $DO_IT == true ]; then

        if [ ${IMAGE_AT_BEAM_CENTRES} == true ] && [ "$DIRECTION_SCI" == "" ]; then
            reference="# No reference image or offsets, as we take the image centres"
        else
            reference="# Reference image for offsets
linmos.feeds.centreref  = 0
linmos.feeds.spacing    = ${LINMOS_BEAM_SPACING}
# Beam offsets
${LINMOS_BEAM_OFFSETS}"
        fi


        setJob linmos_${subband} linmos_${subband}
        cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_LINMOS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-linmos-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/science_linmos_\${SLURM_JOB_ID}.in
log=${logs}/science_linmos_\${SLURM_JOB_ID}.log

# bit of image name before the beam ID
imagePrefix=image.restored.wr.${subband}.${IMAGE_BASE_SPECTRAL}
weightPrefix=weights.wr.${subband}.${IMAGE_BASE_SPECTRAL}
outImage="linmos.\${imagePrefix}"
outWeight="linmos.\${weightPrefix}"
imageList=""
weightList=""

for BEAM in ${BEAMS_TO_USE}; do
    if [ -e \${imagePrefix}.beam\${BEAM} ]; then
        imageList="\${imageList}\${imagePrefix}.beam\${BEAM} "
        weightList="\${imageList}\${weightPrefix}.beam\${BEAM} "
    else
        echo "WARNING: Beam \${BEAM} image not present - not including in mosaic!"
    fi
done

if [ "\${imageList}" != "" ]; then

    imageList=\`echo \${imageList} | sed -e 's/ /,/g' \`
    weightList=\`echo \${weightList} | sed -e 's/ /,/g' \`
    cat > \$parset << EOFINNER
linmos.names            = [\${imageList}]
linmos.weights          = [\${weightList}]
linmos.outname          = \${outImage}
linmos.outWeight        = \${outWeight}
linmos.findmosaics      = false
linmos.weighttype       = FromWeightImages
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.nterms           = ${NUM_TAYLOR_TERMS}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} $linmos -c \$parset > \$log
    err=\$?
    rejuvenate *.${IMAGE_BASE_CONT}*
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
else

    echo "ERROR - no good images were found for mosaicking!"
    writeStats \${SLURM_JOB_ID} linmos FAIL --- --- --- --- --- txt > stats/stats-\${SLURM_JOB_ID}-linmos.txt
    writeStats \${SLURM_JOB_ID} linmos FAIL --- --- --- --- --- csv > stats/stats-\${SLURM_JOB_ID}-linmos.csv

fi
EOFOUTER

        if [ $SUBMIT_JOBS == true ]; then
            FLAG_IMAGING_DEP=`echo $FLAG_IMAGING_DEP | sed -e 's/afterok/afterany/g'`
        ID_LINMOS_SCI=`sbatch $FLAG_IMAGING_DEP $sbatchfile | awk '{print $4}'`
        recordJob ${ID_LINMOS_SCI} "Make a mosaic image of the science observation, with flags \"${FLAG_IMAGING_DEP}\""
        else
            echo "Would make a mosaic image  of the science observation with slurm file $sbatchfile"
        fi

        echo " "

    fi
done

if [ ${DO_SOURCE_FINDING_MOSAIC} == true ]; then
    # Run the sourcefinder on the mosaicked image.

    # set the $imageBase variable to have 'linmos' in it
    BEAM="all"
    setImageBase spectral

    . ${PIPELINEDIR}/sourcefinding.sh

fi
