#!/bin/bash -l
#
# Sets up the continuum-subtraction job for the case where the
# continuum is represented by the clean model image from the continuum
# imaging 
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

setContsubFilenames

# If we're here, then CONTSUB_METHOD=CleanModel
# In this bit, we use the clean model from the continuum imaging
# as the input to ccontsubtract

ContsubModelDefinition="# The model definition
CContsubtract.imagetype                           = ${IMAGETYPE_CONT}
CContsubtract.sources.names                       = [lsm]
CContsubtract.sources.lsm.direction               = \${modelDirection}
CContsubtract.sources.lsm.model                   = ${contsubCleanModelImage%%.taylor.0}
CContsubtract.sources.lsm.nterms                  = ${NUM_TAYLOR_TERMS}"
if [ "${NUM_TAYLOR_TERMS}" -gt 1 ]; then
    if [ "$MFS_REF_FREQ" == "" ]; then
        freq="\${centreFreq}"
    else
        freq=${MFS_REF_FREQ}
    fi
    ContsubModelDefinition="$ContsubModelDefinition
CContsubtract.visweights                          = MFS
CContsubtract.visweights.MFS.reffreq              = ${freq}"
fi

cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_CONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-contsubSLsci-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

if [ "${DIRECTION}" != "" ]; then
    modelDirection="${DIRECTION}"
else
    msMetadata="${MS_METADATA}"
    ra=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=RA)
    dec=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Dec)
    epoch=\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Epoch)
    modelDirection="[\${ra}, \${dec}, \${epoch}]"
fi
centreFreq="\$(python "${PIPELINEDIR}/parseMSlistOutput.py" --file="\$msMetadata" --val=Freq)"

parset="${parsets}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.in"
log="${logs}/contsub_spectralline_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
cat > "\$parset" <<EOFINNER
# The measurement set name - this will be overwritten
CContSubtract.dataset                             = ${msSciSL}
${ContsubModelDefinition}
# The gridding parameters
CContSubtract.gridder.snapshotimaging             = ${GRIDDER_SNAPSHOT_IMAGING}
CContSubtract.gridder.snapshotimaging.wtolerance  = ${GRIDDER_SNAPSHOT_WTOL}
CContSubtract.gridder.snapshotimaging.longtrack   = ${GRIDDER_SNAPSHOT_LONGTRACK}
CContSubtract.gridder.snapshotimaging.clipping    = ${GRIDDER_SNAPSHOT_CLIPPING}
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

NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${ccontsubtract} -c "\${parset}" > "\${log}"
err=\$?
rejuvenate ${msSciSL}
extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch "$CONT_SUB_CHECK_FILE"
fi

EOFOUTER
