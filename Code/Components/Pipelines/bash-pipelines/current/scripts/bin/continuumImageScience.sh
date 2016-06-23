#!/bin/bash -l
#
# Launches a job to image the current beam of the science
# observation, using the BasisfunctionMFS solver.
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

# Define the Cimager parset and associated parameters
. ${PIPELINEDIR}/getContinuumCimagerParams.sh

ID_CONTIMG_SCI=""

DO_IT=$DO_CONT_IMAGING

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${outputImage} ]; then
    if [ $DO_IT == true ]; then
        echo "Image ${outputImage} exists, so not running continuum imaging for beam ${BEAM}"
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/science_continuumImage_beam$BEAM.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_CONT_IMAGE}
#SBATCH --ntasks=${NUM_CPUS_CONTIMG_SCI}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_CONT_IMAGING}
#SBATCH --job-name=clean${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contImaging-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/science_imaging_beam${BEAM}_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
${cimagerParams}
#
# Apply calibration
Cimager.calibrate                               = false
EOFINNER

log=${logs}/science_imaging_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=${NUM_CPUS_CONTIMG_SCI}
NPPN=${CPUS_PER_CORE_CONT_IMAGING}
aprun -n \${NCORES} -N \${NPPN} $cimager -c \$parset > \$log
err=\$?
rejuvenate *.${imageBase}*
rejuvenate ${OUTPUT}/${msSciAv}
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} contImaging_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
	DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
        DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
        DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
        DEP=`addDep "$DEP" "$ID_AVERAGE_SCI"`
	ID_CONTIMG_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_CONTIMG_SCI} "Make a continuum image for beam $BEAM of the science observation, with flags \"$DEP\""
        FLAG_IMAGING_DEP=`addDep "$FLAG_IMAGING_DEP" "$ID_CONTIMG_SCI"`
    else
	echo "Would make a continuum image for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
