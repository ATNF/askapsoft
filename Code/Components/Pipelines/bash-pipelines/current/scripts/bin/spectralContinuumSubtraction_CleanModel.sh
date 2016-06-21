#!/bin/bash -l
#
# Sets up the continuum-subtraction job for the case where the
# continuum is represented by the clean model image from the continuum
# imaging 
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

# If we're here, then CONTSUB_METHOD=CleanModel
# In this bit, we use the clean model from the continuum imaging
# as the input to ccontsubtract

modelImage=image.${imageBase}

cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPECTRAL_CONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=contsubSLsci${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-contsubSLsci-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

if [ "${DIRECTION_SCI}" != "" ]; then
    modelDirection="${DIRECTION_SCI}"
else
    log=${logs}/mslist_for_ccontsub_\${SLURM_JOB_ID}.log
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} $mslist --full ${msSciSL} 1>& \${log}
    ra=\`grep -A1 RA \$log | tail -1 | awk '{print \$7}'\`
    dec=\`grep -A1 RA \$log | tail -1 | awk '{print \$8}'\`
    eq=\`grep -A1 RA \$log | tail -1 | awk '{print \$9}'\`
    modelDirection="[\${ra}, \${dec}, \${eq}]"
fi

parset=${parsets}/contsub_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.in
log=${logs}/contsub_spectralline_beam${BEAM}_\${SLURM_JOB_ID}.log
cat > \$parset <<EOFINNER
# The measurement set name - this will be overwritten
CContSubtract.dataset                             = ${msSciSL}
# The model definition
CContSubtract.sources.names                       = [lsm]
CContSubtract.sources.lsm.direction               = \${modelDirection}
CContSubtract.sources.lsm.model                   = ${modelImage}
CContSubtract.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}
# The gridding parameters
CContSubtract.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
CContSubtract.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
CContSubtract.gridder.snapshotimaging.longtrack   = ${GRIDDER_SNAPSHOT_LONGTRACK}
CContSubtract.gridder                             = WProject
CContSubtract.gridder.WProject.wmax               = ${GRIDDER_WMAX}
CContSubtract.gridder.WProject.nwplanes           = ${GRIDDER_NWPLANES}
CContSubtract.gridder.WProject.oversample         = ${GRIDDER_OVERSAMPLE}
CContSubtract.gridder.WProject.maxfeeds           = 1
CContSubtract.gridder.WProject.maxsupport         = ${GRIDDER_MAXSUPPORT}
CContSubtract.gridder.WProject.frequencydependent = true
CContSubtract.gridder.WProject.variablesupport    = true
CContSubtract.gridder.WProject.offsetsupport      = true
EOFINNER

aprun -n 1 -N 1NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} ${ccontsubtract} -c \${parset} > \${log}
err=\$?
rejuvenate ${msSciSL}
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} contsub_spectral_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch $CONT_SUB_CHECK_FILE
fi

EOFOUTER
