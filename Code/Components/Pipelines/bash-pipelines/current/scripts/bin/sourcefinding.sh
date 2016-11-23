#!/bin/bash -l
#
# Launches a job to create a catalogue of sources in the continuum image.
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

if [ $DO_SOURCE_FINDING == true ]; then

    # get the text that does the FITS conversion - put in $fitsConvertText
    convertToFITStext

    setJob science_selavy selavy
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_SOURCEFINDING}
#SBATCH --ntasks=${NUM_CPUS_SELAVY}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY}
#SBATCH --job-name=${jobname}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-selavy-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

seldir=selavy_${imageBase}
mkdir -p \$seldir

cd \${seldir}
ln -s ${logs} .
ln -s ${parsets} .

HAVE_IMAGES=true
BEAM=$BEAM
NUM_TAYLOR_TERMS=${NUM_TAYLOR_TERMS}

# List of images to convert to FITS in the Selavy job
imlist=""

image=${OUTPUT}/{imageName}
weights=${OUTPUT}/${weightsImage}
imlist="\${imlist} \${image}"
if [ \$NUM_TAYLOR_TERMS -gt 1 ]; then
    t1im=`echo $image | sed -e 's/taylor\.0/taylor\.1/g'`
    if [ -e ${OUTPUT}/\${t1im} ]; then
        imlist="\${imlist} ${OUTPUT}/\${t1im}"
    fi
    t2im=`echo $image | sed -e 's/taylor\.0/taylor\.2/g'`
    if [ -e ${OUTPUT}/\${t2im} ]; then
        imlist="\${imlist} ${OUTPUT}/\${t2im}"
    fi
fi

if [ "\${BEAM}" == "all" ]; then
    imlist="\${imlist} \${weights}"
    weightpars="Selavy.Weights.weightsImage = \${weights##*/}.fits
Selavy.Weights.weightsCutoff = ${LINMOS_CUTOFF}"
else
    weightpars="#"
fi

for im in \${imlist}; do 
    casaim="../\${im##*/}"
    fitsim="../\${im##*/}.fits"
    ${fitsConvertText}
    # make a link so we point to a file in the current directory for Selavy
    if [ ! -e \$fitsim ]; then
        HAVE_IMAGES=false
        echo "ERROR - Could not create \${im}.fits"
    else
        ln -s \${im}.fits .
    fi
    rejuvenate \${casaim}
done

if [ \${HAVE_IMAGES} == true ]; then

    parset=${parsets}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.in
    log=${logs}/science_selavy_${FIELDBEAM}_\${SLURM_JOB_ID}.log
    
    cat > \$parset <<EOFINNER
Selavy.image = \${image##*/}.fits
Selavy.SBid = ${SB_SCIENCE}
Selavy.nsubx = ${SELAVY_NSUBX}
Selavy.nsuby = ${SELAVY_NSUBY}
#
Selavy.snrCut = ${SELAVY_SNR_CUT}
Selavy.flagGrowth = ${SELAVY_FLAG_GROWTH}
Selavy.growthCut = ${SELAVY_GROWTH_CUT}
#
Selavy.VariableThreshold = ${SELAVY_VARIABLE_THRESHOLD}
Selavy.VariableThreshold.boxSize = ${SELAVY_BOX_SIZE}
Selavy.VariableThreshold.ThresholdImageName=detThresh.img
Selavy.VariableThreshold.NoiseImageName=noiseMap.img
Selavy.VariableThreshold.AverageImageName=meanMap.img
Selavy.VariableThreshold.SNRimageName=snrMap.img
\${weightpars}
#
Selavy.Fitter.doFit = true
Selavy.Fitter.fitTypes = [full]
Selavy.Fitter.numGaussFromGuess = true
Selavy.Fitter.maxReducedChisq = 10.
#
Selavy.threshSpatial = 5
Selavy.flagAdjacent = false
#
Selavy.minPix = 3
Selavy.minVoxels = 3
Selavy.minChannels = 1
Selavy.sortingParam = -pflux
EOFINNER

    NCORES=${NUM_CPUS_SELAVY}
    NPPN=${CPUS_PER_CORE_SELAVY}
    aprun -n \${NCORES} -N \${NPPN} $selavy -c \$parset >> \$log
    err=\$?
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} ${jobname} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    fi

else

    echo "FITS conversion failed, so Selavy did not run"

fi

EOFOUTER

    # Dependencies for the job
    DEP=""
    if [ "$FIELD" == "." ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_CONT_ALL"`
    elif [ $BEAM == "all" ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_CONT"`
    else
        if [ $DO_SELFCAL == true ]; then
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI_SC"`
        else
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI"`
        fi
    fi
    
    if [ $SUBMIT_JOBS == true ]; then
	ID_SOURCEFINDING_SCI=`sbatch ${DEP} $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SOURCEFINDING_SCI} "Run the source-finder on the science observation with flags \"$DEP\""
    else
	echo "Would run the source-finder on the science observation with slurm file $sbatchfile"
    fi

    echo " "

    
fi
