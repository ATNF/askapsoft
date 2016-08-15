#!/bin/bash -l
#
# Launches a job to flag the science data in two passes, one with a
# flat amplitude cut to remove any bright spikes, plus antenna or
# autocorrelation-based flagging, and a second with a dynamic
# amplitude threshold
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

ID_FLAG_SCI=""

DO_IT=$DO_FLAG_SCIENCE

if [ -e $FLAG_CHECK_FILE ]; then
    if [ $DO_IT == true ]; then
        echo "Flagging for beam $BEAM of science observation has already been done - not re-doing."
    fi
    DO_IT=false
fi

if [ $DO_IT == true ]; then

    DO_AMP_FLAG=false
    ruleList=""
    
     if [ "$ANTENNA_FLAG_SCIENCE" == "" ]; then
        antennaFlagging="# Not flagging any antennas"
    else
        antennaFlagging="# The following flags out the requested antennas:
Cflag.selection_flagger.rule1.antenna   = ${ANTENNA_FLAG_SCIENCE}"
        if [ "${ruleList}" == "" ]; then
            ruleList="rule1"
        else
            ruleList="${ruleList},rule1"
        fi
        DO_AMP_FLAG=true
     fi

     if [ ${FLAG_AUTOCORRELATION_SCIENCE} == true ]; then
         autocorrFlagging="# The following flags out the autocorrelations, if set to true:
Cflag.selection_flagger.rule2.autocorr  = ${FLAG_AUTOCORRELATION_SCIENCE}"
         if [ "${ruleList}" == "" ]; then
             ruleList="rule2"
         else
             ruleList="${ruleList},rule2"
         fi
         DO_AMP_FLAG=true
     else
         autocorrFlagging="# No flagging of autocorrelations"
     fi

     if [ "${ruleList}" == "" ]; then
         selectionRule="# No selection rules used"
     else
         selectionRule="Cflag.selection_flagger.rules           = [${ruleList}]"
     fi
     
     # The flat amplitude cut to be applied
     if [ ${FLAG_DO_FLAT_AMPLITUDE_SCIENCE} == true ]; then
         amplitudeCut="# Amplitude based flagging
#   Here we apply a simple cut at a given amplitude level
Cflag.amplitude_flagger.enable          = true
Cflag.amplitude_flagger.high            = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE}"
         if [ "${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}" != "" ]; then
             # Only add the low threshold if it has been given
             amplitudeCut="${amplitudeCut}
Cflag.amplitude_flagger.low             = ${FLAG_THRESHOLD_AMPLITUDE_SCIENCE_LOW}"
         fi
         DO_AMP_FLAG=true
     else
         amplitudeCut="# No flat amplitude flagging applied"
     fi
         
   
    sbatchfile=$slurms/flag_science_beam${BEAM}.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_FLAG_SCIENCE}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=flagSci${BEAM}
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-flagSci-b${BEAM}-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

DO_AMP_FLAG=${DO_AMP_FLAG}
if [ \$DO_AMP_FLAG == true ]; then

    parset=${parsets}/cflag_amp_science_beam${BEAM}_\${SLURM_JOB_ID}.in
    cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

${amplitudeCut}

${selectionRule}

${antennaFlagging}

${autocorrFlagging}
EOFINNER

    log=${logs}/cflag_amp_science_beam${BEAM}_\${SLURM_JOB_ID}.log

    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} ${cflag} -c \${parset} > \${log}
    err=\$?
    rejuvenate ${msSci}
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} flagScienceAmp_B${BEAM} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch $FLAG_CHECK_FILE
    fi
fi

DO_DYNAMIC=${FLAG_DO_DYNAMIC_AMPLITUDE_SCIENCE}
if [ \${DO_DYNAMIC} == true ]; then

    parset=${parsets}/cflag_dynamic_science_beam${BEAM}_\${SLURM_JOB_ID}.in
    cat > \$parset <<EOFINNER
# The path/filename for the measurement set
Cflag.dataset                           = ${msSci}

# Amplitude based flagging with dynamic thresholds
#  This finds a statistical threshold in the spectrum of each
#  time-step, then applies the same threshold level to the integrated
#  spectrum at the end.
Cflag.amplitude_flagger.enable           = true
Cflag.amplitude_flagger.dynamicBounds    = true
Cflag.amplitude_flagger.threshold        = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE}
Cflag.amplitude_flagger.integrateSpectra = true
Cflag.amplitude_flagger.integrateSpectra.threshold = ${FLAG_THRESHOLD_DYNAMIC_SCIENCE}
EOFINNER
    
    log=${logs}/cflag_dynamic_science_beam${BEAM}_\${SLURM_JOB_ID}.log
    
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} ${cflag} -c \${parset} > \${log}
    err=\$?
    rejuvenate ${msSci}
    extractStats \${log} \${NCORES} \${SLURM_JOB_ID} \${err} flagScienceDynamic_B${BEAM} "txt,csv"
    if [ \$err != 0 ]; then
        exit \$err
    else
        touch $FLAG_CHECK_FILE
    fi
    
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        DEP=""
        DEP=`addDep "$DEP" "$DEP_START"`
        DEP=`addDep "$DEP" "$ID_SPLIT_SCI"`
	ID_FLAG_SCI=`sbatch $DEP $sbatchfile | awk '{print $4}'`
	recordJob ${ID_FLAG_SCI} "Flagging beam ${BEAM} of science observation"
    else
	echo "Would run flagging beam ${BEAM} for science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
