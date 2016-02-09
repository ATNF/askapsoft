#!/usr/bin/env bash
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
    
    if [ $NUM_TAYLOR_TERMS == 1 ]; then
        image=${OUTPUT}/image.${imageBase}.restored
        weights=${OUTPUT}/weights.${imageBase}
    else
        image=${OUTPUT}/image.${imageBase}.taylor.0.restored
        weights=${OUTPUT}/weights.${imageBase}.taylor.0
    fi

    if [ $BEAM == "all" ]; then
        weightpars="Selavy.Weights.weightsImage = $weights
Selavy.Weights.weightsCutoff = ${LINMOS_CUTOFF}"
    else
        weightpars="#"
    fi

    sbatchfile=$slurms/science_selavy_B${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/usr/bin/env bash
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${RESERVATION_REQUEST}
#SBATCH --time=12:00:00
#SBATCH --ntasks=${NUM_CPUS_SELAVY}
#SBATCH --ntasks-per-node=${CPUS_PER_CORE_SELAVY}
#SBATCH --job-name=selavyB${BEAM}
${EMAIL_REQUEST}
#SBATCH --export=ASKAP_ROOT,AIPSPATH
#SBATCH --output=$slurmOut/slurm-selavy-%j.out

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

parset=${parsets}/science_selavy_B${BEAM}_\${SLURM_JOB_ID}.in
log=${logs}/science_selavy_B${BEAM}_\${SLURM_JOB_ID}.log

cat > \$parset <<EOFINNER
Selavy.image = ${image}
Selavy.SBid = 1248
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
${weightpars}
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

aprun -n ${NUM_CPUS_SELAVY} -N ${CPUS_PER_CORE_SELAVY} $selavy -c \$parset >> \$log
err=\$?
NUM_CPUS=${NUM_CPUS_SELAVY}
extractStats \${log} \${SLURM_JOB_ID} \${err} selavy_B${BEAM} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
fi
EOFOUTER

    # Dependencies for the job
    DEP=""
    if [ $BEAM == "all" ]; then
        DEP=`addDep "$DEP" "$ID_LINMOS_SCI"`
    else
        if [ $DO_SELFCAL == true ]; then
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI_SC"`
        else
            DEP=`addDep "$DEP" "$ID_CONTIMG_SCI"`
        fi
    fi
    
    if [ $SUBMIT_JOBS == true ]; then
	ID_SOURCEFINDING_SCI=`sbatch ${DEP} $sbatchfile | awk '{print $4}'`
	recordJob ${ID_SOURCEFINDING_SCI} "Run the source-finder on the science observation"
    else
	echo "Would run the source-finder on the science observation with slurm file $sbatchfile"
    fi

    echo " "

    
fi
