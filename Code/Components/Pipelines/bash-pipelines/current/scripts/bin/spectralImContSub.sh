#!/bin/bash -l
#
# Runs a job to do image-based continuum subtraction on the current
# spectral cube, using the ACES tasks in either
# $ACES/tools/robust_contsub.py or $ACES/tools/contsub_im.py. The
# former fits a low-order polynomial to each spectrum, subtracting it
# from the image. The later uses a Savitzky-Golay filter to find &
# remove the spectral baseline For an input cube
# image.something.restored, it produces
# image.something.restored.contsub, holding the continuum-subtracted
# data, and image.something.restored.coefs, holding the polynomial
# coefficients for each spectrum (only for the robust_contsub.py
# case). 
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

    contsubName=${imageName}
    
    # Make sure we can see the robust_contsub script
    script_location="${ACES_LOCATION}/tools"
    if [ ! -e "${script_location}/${SPECTRAL_IMSUB_SCRIPT}" ]; then
        echo "WARNING - ${SPECTRAL_IMSUB_SCRIPT} not found in $script_location - not running image-based continuum subtraction."
        DO_IT=false
        DO_SPECTRAL_IMSUB=false
    fi
    
    if [ "${DO_IT}" == "true" ]; then

        # Make a working directory - the casapy & ipython log files
        # will go in here. This will prevent conflicts between jobs
        # that start at the same time
        workingDirectory="imcontsub-working-beam${BEAM}"
        if [ ${NUM_SPECTRAL_CUBES} -gt 1 ]; then
            workingDirectory="${workingDirectory}.${subband}"
        fi

        # Set the $imageName for the restored cube, as we will use this in the slurm job
        imageCode=restored
        setImageProperties spectral

        setJob "spectral_imcontsub${subband}" "imcontsub${subband}"

        if [ "${ACES_VERSION_USED}" -gt 47195 ]; then

            setup="# Setting up command-line arguments for contsub
    args=\"--image=../${imageName}\""
            if [ "${SPECTRAL_IMSUB_VERBOSE}" == "true" ]; then
                setup="${setup}
    args=\"\${args} -v\""
            fi
            
            if [ "${SPECTRAL_IMSUB_SCRIPT}" == "contsub_im.py" ]; then
                setup="${setup}
    args=\"\${args} --filterwidth=${SPECTRAL_IMSUB_SG_FILTERWIDTH}\"
    args=\"\${args} --binwidth=${SPECTRAL_IMSUB_SG_BINWIDTH}\""

            else
                if [ "${SPECTRAL_IMSUB_SCRIPT}" != "robust_contsub.py" ]; then
                    echo "SPECTRAL_IMSUB_SCRIPT - only \"robust_contsub.py\" or \"contsub_im.py\" allowed."
                    echo "                      - using \"robust_contsub.py\""
                    SPECTRAL_IMSUB_SCRIPT="robust_contsub.py"
                fi
                setup="${setup}
    args=\"\${args} --threshold=${SPECTRAL_IMSUB_THRESHOLD}\"
    args=\"\${args} --order=${SPECTRAL_IMSUB_FIT_ORDER}\"
    args=\"\${args} --n_every=${SPECTRAL_IMSUB_CHAN_SAMPLING}\"
    args=\"\${args} --log_every=${SPECTRAL_IMSUB_LOG_SAMPLING}\""
            fi
                
            cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMCONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-imcontsubSL-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. "${PIPELINEDIR}/utils.sh"

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

BEAM=${BEAM}
imageName=${imageName}
contsubName=${contsubName}

if [ ! -e "\${imageName}" ]; then

    echo "Image cube \${imageName} does not exist."
    echo "Not running image-based continuum subtraction"

else

    # Make a working directory - the casapy & ipython log files will go in here.
    # This will prevent conflicts
    workdir=${workingDirectory}
    mkdir -p \$workdir
    cd \$workdir

    log="${logs}/spectral_imcontsub_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    STARTTIME=\$(date +%FT%T)

    ${setup}
    script="${script_location}/${SPECTRAL_IMSUB_SCRIPT}"

    NCORES=1
    NPPN=1
    loadModule casa
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o \${log}.timing casa --nogui --nologger --log2term -c "\${script}" \${args} > "\${log}"
    err=\$?
    unloadModule casa
    cd ..
    rejuvenate "\${imageName}"
    echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
    extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "${jobname}" "txt,csv"  

    if [ \$err != 0 ]; then
        exit \$err
    fi

    # Need to convert the contsub image to FITS if we are working in
    #   FITS format
    IMAGETYPE_SPECTRAL=${IMAGETYPE_SPECTRAL}
    if [ "\${IMAGETYPE_SPECTRAL}" == "fits" ]; then
        echo "Converting contsub images to FITS"
        casaim="\${contsubName%%.fits}"
        fitsim="\${contsubName%%.fits}.fits"
        parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
        log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
        ${fitsConvertText}
    fi
    
    # Find the cube statistics
    loadModule mpi4py
    echo "Finding cube stats for \${contsubName}"
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${contsubName}


fi

EOF
            
        else
        
            cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_SPECTRAL_IMCONTSUB}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-imcontsubSL-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. "${PIPELINEDIR}/utils.sh"

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

BEAM=${BEAM}
imageName=${imageName}
contsubName=${contsubName}

if [ ! -e "\${imageName}" ]; then

    echo "Image cube \${imageName} does not exist."
    echo "Not running image-based continuum subtraction"

else

    # Make a working directory - the casapy & ipython log files will go in here.
    # This will prevent conflicts
    workdir=${workingDirectory}
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
    log="${logs}/spectral_imcontsub_${FIELDBEAM}_\${SLURM_JOB_ID}.log"
    STARTTIME=\$(date +%FT%T)
    NCORES=1
    NPPN=1
    loadModule casa
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" casa --nogui --nologger --log2term -c "\${pyscript}" > "\${log}"
    err=\$?
    unloadModule casa
    cd ..

    rejuvenate "\${imageName}"
    echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
    extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "${jobname}" "txt,csv"  

    if [ \$err != 0 ]; then
        exit \$err
    fi
    
    # Need to convert the contsub image to FITS if we are working in
    #   FITS format
    IMAGETYPE_SPECTRAL=${IMAGETYPE_SPECTRAL}
    if [ "\${IMAGETYPE_SPECTRAL}" == "fits" ]; then
        echo "Converting contsub images to FITS"
        casaim="\${contsubName%%.fits}"
        fitsim="\${contsubName%%.fits}.fits"
        parset="${parsets}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.in"
        log="${logs}/convertToFITS_\${casaim##*/}_\${SLURM_JOB_ID}.log"
        ${fitsConvertText}
    fi

    # Find the cube statistics
    loadModule mpi4py
    echo "Finding cube stats for \${contsubName}"
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} \${PIPELINEDIR}/findCubeStatistics.py -c \${contsubName}


fi
EOF

        fi
            
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
