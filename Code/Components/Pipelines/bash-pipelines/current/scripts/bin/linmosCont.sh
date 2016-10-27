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

ID_LINMOS_CONT=""

DO_IT=$DO_MOSAIC

# Get the name of the mosaicked image
imageCode=restored
BEAM=all
setImageProperties cont

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${imageName} ]; then
    if [ $DO_IT == true ]; then
        echo "Image ${imageName} exists, so not running continuum mosaicking"
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

    
    setJob linmos linmos
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

nterms=${NUM_TAYLOR_TERMS}
IMAGE_BASE_CONT=${IMAGE_BASE_CONT}
FIELD=${FIELD}

for imageCode in restored altrestored image residual; do 

    for((TERM=0;TERM<\${nterms};TERM++)); do

        beamList=""
        for BEAM in ${BEAMS_TO_USE}; do
            setImageProperties cont
            echo \$imageName
            if [ -e \${imageName} ]; then
                if [ "\${beamList}" == "" ]; then
                    beamList="\${imageName}"
                else
                    beamList="\${beamList},\${imageName}"
                fi
            fi
        done
        if [ "\${beamList}" != "" ]; then
            BEAM=all
            setImageProperties cont
            echo "Mosaicking to form \${imageName}"
            parset=${parsets}/science_linmos_${FIELDBEAM}_\${imageCode}_\${SLURM_JOB_ID}.in
            log=${logs}/science_linmos_${FIELDBEAM}_\${imageCode}_\${SLURM_JOB_ID}.log
            cat > \${parset} << EOFINNER
linmos.names            = [\${beamList}]
linmos.outname          = \$imageName
linmos.outweight        = \$weightsImage
linmos.weighttype       = FromPrimaryBeamModel
linmos.weightstate      = Inherent
${reference}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

            NCORES=1
            NPPN=1
            aprun -n \${NCORES} -N \${NPPN} $linmos -c \$parset > \$log
            err=\$?
            for im in `echo \${beamList} | sed -e 's/,/ /g'`; do
                rejuvenate \${im}
            done
            extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname}_\${imageCode} "txt,csv"
            if [ \$err != 0 ]; then
                exit \$err
            fi
        else
            echo "ERROR - no good images were found for mosaicking image type '\${imageCode}'!"
            writeStats \${SLURM_JOB_ID} linmos_\${imageCode} FAIL --- --- --- --- --- txt > stats/stats-\${SLURM_JOB_ID}-linmos.txt
            writeStats \${SLURM_JOB_ID} linmos_\${imageCode} FAIL --- --- --- --- --- csv > stats/stats-\${SLURM_JOB_ID}-linmos.csv
        fi
    done
done
EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        FLAG_IMAGING_DEP=`echo $FLAG_IMAGING_DEP | sed -e 's/afterok/afterany/g'`
	ID_LINMOS_CONT=`sbatch $FLAG_IMAGING_DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_LINMOS_CONT} "Make a mosaic image of the science observation, field $FIELD, with flags \"${FLAG_IMAGING_DEP}\""
        FULL_LINMOS_DEP=`addDep "${FULL_LINMOS_DEP}" "${ID_LINMOS_CONT}"`
    else
	echo "Would make a mosaic image  of the science observation, field $FIELD with slurm file $sbatchfile"
    fi

    echo " "

fi


if [ ${DO_SOURCE_FINDING_MOSAIC} == true ]; then
    # Run the sourcefinder on the mosaicked image.

    # set the $imageBase variable for the mosaicked image
    BEAM="all"
    setImageBase cont

    . ${PIPELINEDIR}/sourcefinding.sh

fi
