#!/bin/bash -l
#
# Launches a job to convert any CASA images into FITS
# format. This job is launched as an array job, with one job for each
# potential image (this list is defined when the processASKAP script is
# called, so the CASA images don't necessarily exist - if they don't
# exist at run-time, nothing is done by that individual job).
# The FITS headers are populated with the PROJECT keyword.
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

if [ "${DO_CONVERT_TO_FITS}" == "true" ]; then

    sbatchfile="$slurms/convert_to_FITS.sbatch"
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_FITS_CONVERT}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=FITSconvert
${exportDirective}
#SBATCH --output="$slurmOut/slurm-FITSconvert-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

expectedImageNames=()

# Define the lists of image names, types, 
. ${getArtifacts}

expectedImageNames=(\${casdaTwoDimImageNames[@]})
expectedImageNames+=(\${casdaOtherDimImageNames[@]})

echo "Image names = \${expectedImageNames[@]}"

for((i=0;i<\${#expectedImageNames[@]};i++)); do

    image=\${expectedImageNames[i]}
    echo "Launching conversion job for \$image"
    casaim=\${image%%.fits}
    fitsim=\${image%%.fits}.fits
    
    parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
    log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
    
    ${fitsConvertText}

done

EOFOUTER
    
    if [ "${SUBMIT_JOBS}" == "true" ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterany:$(echo "$ALL_JOB_IDS" | sed -e 's/,/:/g')"
        fi
        ID_FITSCONVERT=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_FITSCONVERT}" "Job to convert remaining images to FITS, with flags \"${dep}\""
    else
        echo "Would submit job to convert remaining images to FITS, with slurm file $sbatchfile"
    fi




fi
