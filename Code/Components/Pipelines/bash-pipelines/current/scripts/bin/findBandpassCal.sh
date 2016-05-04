#!/bin/bash -l
#
# Launches a job to solve for the bandpass calibration. This uses all
# 1934-638 measurement sets after the splitFlag1934.sh jobs have
# completed. The bandpass calibration is done assuming the special
# '1934-638' component.
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

ID_CBPCAL=""

DO_IT=$DO_FIND_BANDPASS

if [ $CLOBBER == false ] && [ -e ${OUTPUT}/${TABLE_BANDPASS} ]; then
    if [ $DO_IT == true ]; then
        echo "Bandpass table ${TABLE_BANDPASS} exists, so not running cbpcalibrator"
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/cbpcalibrator_1934.sbatch
    cat > $sbatchfile <<EOF
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=${NUM_CPUS_CBPCAL}
#SBATCH --ntasks-per-node=20
#SBATCH --job-name=cbpcal
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-findBandpass-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

parset=${parsets}/cbpcalibrator_1934_\${SLURM_JOB_ID}.in
cat > \$parset <<EOFINNER
Cbpcalibrator.dataset                         = [${ms1934list}]
Cbpcalibrator.nAnt                            = ${NUM_ANT}
Cbpcalibrator.nBeam                           = ${nbeams}
Cbpcalibrator.nChan                           = ${NUM_CHAN_1934}
Cbpcalibrator.refantenna                      = 1
#
Cbpcalibrator.calibaccess                     = table
Cbpcalibrator.calibaccess.table.maxant        = ${NUM_ANT}
Cbpcalibrator.calibaccess.table.maxbeam       = ${maxbeam}
Cbpcalibrator.calibaccess.table.maxchan       = ${NUM_CHAN_1934}
Cbpcalibrator.calibaccess.table               = ${TABLE_BANDPASS}
#
Cbpcalibrator.sources.names                   = [field1]
Cbpcalibrator.sources.field1.direction        = ${DIRECTION_1934}
Cbpcalibrator.sources.field1.components       = src
Cbpcalibrator.sources.src.calibrator          = 1934-638
#
Cbpcalibrator.gridder                         = SphFunc
#
Cbpcalibrator.ncycles                         = ${NCYCLES_BANDPASS_CAL}

EOFINNER

log=${logs}/cbpcalibrator_1934_\${SLURM_JOB_ID}.log

NCORES=${NUM_CPUS_CBPCAL}
NPPN=20
aprun -n \${NCORES} -N \${NPPN} $cbpcalibrator -c \$parset > \$log
err=\$?
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} findBandpass "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOF

    if [ $SUBMIT_JOBS == true ]; then
        ID_CBPCAL=`sbatch ${FLAG_CBPCAL_DEP} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_CBPCAL} "Finding bandpass calibration with 1934-638, with flags \"$FLAG_CBPCAL_DEP\""
    else
        echo "Would find bandpass calibration with slurm file $sbatchfile"
    fi

    echo " "

fi
