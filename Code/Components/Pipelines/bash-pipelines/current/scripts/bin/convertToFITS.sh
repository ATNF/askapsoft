#!/bin/bash -l
#
# Launches a job to convert any CASA images into FITS
# format. This job is launched as an array job, with one job for each
# potential image (this list is defined when the processBETA script is
# called, so the CASA images don't necessarily exist - if they don't
# exist at run-time, nothing is done by that individual job).
# The FITS headers are populated with the PROJECT keyword.
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

if [ ${DO_CONVERT_TO_FITS} == true ]; then

    expectedImageNames=()

    beamlist="${BEAMS_TO_USE} all"
    for BEAM in $beamlist; do

        setImageBaseCont
        for((i=0;i<${NUM_TAYLOR_TERMS};i++)); do

            if [ ${NUM_TAYLOR_TERMS} -eq 1 ]; then
                imBase=${imageBase}
            else
                imBase="${imageBase}.taylor.${i}"
            fi
            expectedImageNames+=(image.${imBase}.restored)
            for im in ${IMAGE_LIST}; do

                if [ -e ${imagename} ]; then
                    expectedImageNames+=(${im}.${imBase})
                fi
            done
        done

        setImageBaseSpectral
        expectedImageNames+=(image.${imBase}.restored)
        for im in ${IMAGE_LIST}; do

            if [ -e ${imagename} ]; then
                expectedImageNames+=(${im}.${imBase})
            fi
        done

    done
    
    sbatchfile="$slurms/convert_to_FITS.sbatch"
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_FITS_CONVERT}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=FITSconvert
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-FITSconvert-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

imageList=(${expectedImageNames[@]})
casaim="\${imageList[\${SLURM_ARRAY_TASK_ID}]}"
fitsim="\${imageList[\${SLURM_ARRAY_TASK_ID}]}.fits"

if [ -e \${casaim} ] && [ ! -e \${fitsim} ]; then
    # The FITS version of this image doesn't exist

    aprun -n 1 $image2fits in=\${casaim} out=\${fitsim}

    script=$parsets/fixheader_\${casaim}_\${SLURM_JOB_ID}.py
    log=$logs/fixheader_\${casaim}_\${SLURM_JOB_ID}.log
    cat > \$script << EOFSCRIPT
#!/usr/bin/env python
import astropy.io.fits as fits
image='\${fitsim}'
project='${PROJECT_ID}'
hdulist = fits.open(image,'update')
hdulist[0].header.set('PROJECT',project)
hdulist.flush()
EOFSCRIPT

    aprun -n 1 python \$script > \$log

fi

EOFOUTER
    
    if [ $SUBMIT_JOBS == true ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:`echo $ALL_JOB_IDS | sed -e 's/,/:/g'`"
        fi
        numImages=${#expectedImageNames[@]}
        arrayIndices=`echo $numImages | awk '{printf "0-%d",$1-1}'`
        ID_FITSCONVERT=`sbatch --array=${arrayIndices} ${dep} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_FITSCONVERT} "Job to convert remaining images to FITS, with flags \"${dep}\""
    else
        echo "Would submit job to convert remaining images to FITS, with slurm file $sbatchfile"
    fi




fi
