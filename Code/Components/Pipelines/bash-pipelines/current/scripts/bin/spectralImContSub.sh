#!/bin/bash -l
#
# Runs a job to do image-based continuum subtraction on the current
# spectral cube, using the ACES tasks in
# $ACES/tools/robust_contsub.py. This fits a low-order polynomial to
# each spectrum, subtracting it from the image.
# For an input cube image.something.restored, it produces
# image.something.restored.contsub, holding the continuum-subtracted
# data, and image.something.restored.coefs, holding the polynomial
# coefficients for each spectrum 
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

# set the $imageBase variable
setImageBaseSpectral

DO_IT=$DO_SPECTRAL_IMSUB
if [ $CLOBBER == false ] && [ -e ${OUTPUT}/image.${imageBase}.restored.contsub ]; then
    if [ $DO_IT == true ]; then
        echo "Continuum-subtracted spectral cube image.${imageBase}.restored.contsub exists. Not re-doing."
    fi
    DO_IT=false
fi

script_location="$ACES/tools"
script_name=robust_contsub
if [ ! -e ${script_location}/${script_name}.py ]; then
    echo "WARNING - ${script_name}.py not found in $script_location - not running image-based continuum subtraction."
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    sbatchfile=$slurms/spectral_imcontsub_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOF
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMCONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=imcontsubSL${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-imcontsubSL-%j.out

${askapsoftModuleCommands}
module load aces

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

pyscript=${parsets}/spectral_imcontsub_beam${BEAM}_\${SLURM_JOB_ID}.py
cat > \$pyscript << EOFINNER
#!/usr/bin/env python

# Need to import this from ACES
import sys
sys.path.append('${script_location}')
from ${script_name} import robust_contsub

rc=robust_contsub()
rc.poly(infile="image.${imageBase}.restored",threshold=${SPECTRAL_IMSUB_THRESHOLD},verbose=True,fit_order=${SPECTRAL_IMSUB_FIT_ORDER},n_every=${SPECTRAL_IMSUB_CHAN_SAMPLING},log_every=10)

EOFINNER
log=${logs}/spectral_imcontsub_beam${BEAM}_\${SLURM_JOB_ID}.log

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} -b casa --nogui --nologger --log2term -c \${pyscript} > \${log}
err=\$?
rejuvenate image.${imageBase}.restored
extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} imcontsub_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi

EOF

    if [ $SUBMIT_JOBS == true ]; then
        submitIt=true
        if [ $DO_SPECTRAL_IMAGING != true ]; then
            # If we aren't creating the cube in this pipeline run, then check to see if it exists.
            # If it doesn't, we can't run this job.
            if [ ! -e "image.${imageBase}.restored" ]; then
                submitIt=false
                echo "Not submitting image-based continuum subtraction, since the cube image.${imageBase}.restored doesn't exist."
            fi
        fi
        if [ $submitIt == true ]; then
            DEP=""
            DEP=`addDep "$DEP" "$DEP_START"`
            DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
            DEP=`addDep "$DEP" "$ID_FLAG_SCI"`
            DEP=`addDep "$DEP" "$ID_CCALAPPLY_SCI"`
            DEP=`addDep "$DEP" "$ID_SPLIT_SL_SCI"`
            DEP=`addDep "$DEP" "$ID_CAL_APPLY_SL_SCI"`
            DEP=`addDep "$DEP" "$ID_CONT_SUB_SL_SCI"`
            DEP=`addDep "$DEP" "$ID_SPECIMG_SCI"`
            ID_SPEC_IMCONTSUB_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
            recordJob ${ID_SPEC_IMCONTSUB_SCI} "Image-based continuum subtraction for beam $BEAM of the science observation, with flags \"$DEP\""
        fi
    else
        echo "Would run image-based continuum subtraction for beam $BEAM of the science observation with slurm file $sbatchfile"
    fi

    echo " "

fi

