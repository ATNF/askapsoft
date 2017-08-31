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

for subband in ${SUBBAND_WRITER_LIST}; do

    DO_IT=$DO_SPECTRAL_IMSUB

    # Check for the existence of the output - don't overwrite if CLOBBER!=true
    imageCode=contsub
    setImageProperties spectral
    if [ "${CLOBBER}" != "true" ] && [ -e "${imageName}" ]; then
        if [ "${DO_IT}" == "true" ]; then
            echo "Continuum-subtracted spectral cube ${imageName} exists. Not re-doing."
        fi
        DO_IT=false
    fi

    # Make sure we can see the robust_contsub script
    script_location="${ACES_LOCATION}/tools"
    script_name=robust_contsub
    if [ ! -e "${script_location}/${script_name}.py" ]; then
        echo "WARNING - ${script_name}.py not found in $script_location - not running image-based continuum subtraction."
        DO_IT=false
    fi
    
    if [ "${DO_IT}" == "true" ]; then

        setJob "spectral_imcontsub${subband}" "imcontsub${subband}"
        cat > "$sbatchfile" <<EOF
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMCONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-imcontsubSL-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. "${PIPELINEDIR}/utils.sh"

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"

BEAM=${BEAM}
imageName=${imageName}

if [ ! -e "\${imageName}" ]; then

    echo "Image cube \${imageName} does not exist."
    echo "Not running image-based continuum subtraction"

else

    # Make a working directory - the casapy & ipython log files will go in here.
    # This will prevent conflicts
    workdir=imcontsub-working-beam\${BEAM}
    mkdir -p \$workdir
    cd \$workdir

    pyscript=${parsets}/spectral_imcontsub_${FIELDBEAM}_\${SLURM_JOB_ID}.py
    cat > "\$pyscript" << EOFINNER
#!/usr/bin/env python

# Need to import this from ACES
import sys
sys.path.append('${script_location}')
from ${script_name} import robust_contsub

image="../\${imageName}"
threshold=${SPECTRAL_IMSUB_THRESHOLD}
fit_order=${SPECTRAL_IMSUB_FIT_ORDER}
n_every=${SPECTRAL_IMSUB_CHAN_SAMPLING}
rc=robust_contsub()
rc.poly(infile=image,threshold=threshold,verbose=True,fit_order=fit_order,n_every=n_every,log_every=10)

EOFINNER
    log=${logs}/spectral_imcontsub_${FIELDBEAM}_\${SLURM_JOB_ID}.log

    NCORES=1
    NPPN=1
    loadModule casa
    aprun -n \${NCORES} -N \${NPPN} -b casa --nogui --nologger --log2term -c "\${pyscript}" > "\${log}"
    err=\$?
    unloadModule casa
    cd ..
    rejuvenate "\${imageName}"
    #extractStats "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi
    
    if [ "\${imageName%%.fits}" != "\${imageName}" ]; then
        # Want image.contsub.fits, not image.fits.contsub
        echo "Renaming \${imageName}.contsub to \${imageName%%.fits}.contsub.fits"
        mv \${imageName}.contsub \${imageName%%.fits}.contsub.fits
    fi

fi
EOF

        if [ "${SUBMIT_JOBS}" == "true" ]; then
            submitIt=true
            if [ "${DO_SPECTRAL_IMAGING}" != "true" ]; then
                # If we aren't creating the cube in this pipeline run, then check to see if it exists.
                # If it doesn't, we can't run this job.
                imageCode=restored
                setImageProperties spectral
                if [ ! -e "${imageName}" ]; then
                    submitIt=false
                    echo "Not submitting image-based continuum subtraction, since the cube ${imageName} doesn't exist."
                fi
            fi
            if [ "$submitIt" == "true" ]; then
                DEP=""
                DEP=$(addDep "$DEP" "$DEP_START")
                DEP=$(addDep "$DEP" "$ID_SPLIT_SCI")
                DEP=$(addDep "$DEP" "$ID_FLAG_SCI")
                DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI")
                DEP=$(addDep "$DEP" "$ID_SPLIT_SL_SCI")
                DEP=$(addDep "$DEP" "$ID_CAL_APPLY_SL_SCI")
                DEP=$(addDep "$DEP" "$ID_CONT_SUB_SL_SCI")
                DEP=$(addDep "$DEP" "$ID_SPECIMG_SCI")
                ID_SPEC_IMCONTSUB_SCI=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
                DEP_SPECIMCONTSUB=$(addDep "${DEP_SPECIMCONTSUB}" "${ID_SPEC_IMCONTSUB_SCI}")
                recordJob "${ID_SPEC_IMCONTSUB_SCI}" "Image-based continuum subtraction for beam $BEAM of the science observation, with flags \"$DEP\""
            fi
        else
            echo "Would run image-based continuum subtraction for beam $BEAM of the science observation with slurm file $sbatchfile"
        fi

    fi
done
echo " "
