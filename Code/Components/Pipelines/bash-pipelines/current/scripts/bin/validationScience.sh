# @file
#
# Run assorted validation tasks on the science field data
#
# @copyright (c) 2018 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the License,
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
if [ "${DO_VALIDATION_SCIENCE}" == "true" ]; then
    
    sbatchfile="$slurms/validationScience.sbatch"
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_VALIDATE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=validateSci
${exportDirective}
#SBATCH --output="$slurmOut/slurm-validationScience-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

diagnostics=$diagnostics
FIELD_LIST="${FIELD_LIST}"
POL_LIST="${POL_LIST}"
BEAM=00
IMAGE_BASE_CONTCUBE=${IMAGE_BASE_CONTCUBE}
IMAGE_BASE_SPECTRAL=${IMAGE_BASE_SPECTRAL}
DO_ALT_IMAGER_CONTCUBE=${DO_ALT_IMAGER_CONTCUBE}
DO_ALT_IMAGER_SPECTRAL=${DO_ALT_IMAGER_SPECTRAL}
IMAGETYPE_CONTCUBE=${IMAGETYPE_CONTCUBE}
IMAGETYPE_SPECTRAL=${IMAGETYPE_SPECTRAL}
ALT_IMAGER_SINGLE_FILE=${ALT_IMAGER_SINGLE_FILE}
ALT_IMAGER_SINGLE_FILE_CONTCUBE=${ALT_IMAGER_SINGLE_FILE_CONTCUBE}
PROJECT_ID=${PROJECT_ID}

IFS="${IFS_FIELDS}"
for FIELD in \${FIELD_LIST}; do

    # diagnostics directory for individual beam data
    fieldDir=\${diagnostics}/cubestats-${FIELD}
    mkdir -p \$fieldDir

    # copy the cubestats, the plots thereof, and the beam logs to the
    # field directory in diagnostics (but only the ones that exist).
    files=(\${FIELD}/cubeStats*txt \${FIELD}/cubePlot*png \${FIELD}/beamlog*) 
    if [ -e "\${files[0]}" ]; then 
        for f in \${files[@]}; do 
            cp \$f \$fieldDir 
        done 
    fi

    # Cube stats and restoring beam for the spectral cubes
    imageCode=restored
    setImageProperties spectral
    beamwiseCubeStats.py -c \${FIELD}/\${imageName}
    beamwisePSFstats.py -c \${FIELD}/\${imageName}
    imageCode=residual
    setImageProperties spectral
    beamwiseCubeStats.py -c \${FIELD}/\${imageName}
    imageCode=contsub
    setImageProperties spectral
    beamwiseCubeStats.py -c \${FIELD}/\${imageName}

    # Cube stats and restoring beam for the continuum cubes, iterating over polarisation
    IFS="${IFS_DEFAULT}"
    for POLN in \${POL_LIST}; do

        # make a lower-case version of the polarisation, for image name
        pol=\$(echo "\$POLN" | tr '[:upper:]' '[:lower:]')

        imageCode=restored
        setImageProperties contcube
        beamwiseCubeStats.py -c \${FIELD}/\${imageName}
        beamwisePSFstats.py -c \${FIELD}/\${imageName}
        imageCode=residual
        setImageProperties contcube
        beamwiseCubeStats.py -c \${FIELD}/\${imageName}

    done     

done

# copy the new plots to the diagnostics directory (but only if they exist)
files=(beamNoise* beamMin* beamPSF*)
if [ -e "\${files[0]}" ]; then
    for f in \${files[@]}; do
        cp \$f $diagnostics
    done
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterany:$(echo "${ALL_JOB_IDS}" | sed -e 's/,/:/g')"
        fi
        ID_VALIDATION=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_VALIDATION}" "Job to run science-field validation, with flags \"${dep}\""
    else
        echo "Would submit job to run science-field validation, with slurm file $sbatchfile"
    fi



fi
