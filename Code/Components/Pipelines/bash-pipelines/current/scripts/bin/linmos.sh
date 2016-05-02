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

if [ $DO_MOSAIC == true ]; then

    sbatchfile=$slurms/science_linmos.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=linmos
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-linmos-%j.out

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/science_linmos_\${SLURM_JOB_ID}.in
log=${logs}/science_linmos_\${SLURM_JOB_ID}.log

# bit of image name before the beam ID
imagePrefix=image.${IMAGE_BASE_CONT}
# bit of image name after the beam ID
nterms=${NUM_TAYLOR_TERMS}
if [ \${nterms} -gt 1 ]; then
    imageSuffix=taylor.0.restored
else
    imageSuffix=restored
fi
beamList=""
for((beam=${BEAM_MIN};beam<=${BEAM_MAX};beam++)); do
    if [ -e \${imagePrefix}.beam\${beam}.\${imageSuffix} ]; then
        beamList="\${beamList}beam\${beam} "
    else
        echo "WARNING: Beam \${beam} image not present - not including in mosaic!"
    fi 
done

if [ "\${beamList}" != "" ]; then

    beamList=\`echo \${beamList} | sed -e 's/ /,/g' \`

    cat > \$parset << EOFINNER
linmos.names            = [\${beamList}]
linmos.findmosaics      = true
linmos.weighttype       = FromPrimaryBeamModel
linmos.weightstate      = Inherent
linmos.feeds.centreref  = 0
linmos.feeds.spacing    = ${LINMOS_BEAM_SPACING}
${LINMOS_BEAM_OFFSETS}
linmos.psfref           = ${LINMOS_PSF_REF}
linmos.nterms           = ${NUM_TAYLOR_TERMS}
linmos.cutoff           = ${LINMOS_CUTOFF}
EOFINNER

    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} $linmos -c \$parset > \$log
    err=\$?
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} linmos "txt,csv"
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

# Run the sourcefinder on the mosaicked image.
sedstr="s/beam${BEAM_MAX}/linmos/g"
imageBase=`echo $imageBase | sed -e $sedstr`

if [ ${DO_SOURCE_FINDING_MOSAIC} == true ]; then

    BEAM="all"

    . ${PIPELINEDIR}/sourcefinding.sh

fi
