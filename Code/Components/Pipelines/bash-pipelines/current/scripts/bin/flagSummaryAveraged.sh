#!/bin/bash -l
#
# Launches a job to generate flag summary of the averaged science data.
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
# @author Wasim Raja <Wasim.Raja@csiro.au>
#

ID_FLAG_SUMMARY_AVE=""
FLAG_SUMMARY_AVERAGE_FILE=$(realpath ${msSciAvFull}).flagSummary

DO_IT=$DO_FLAG_SUMMARY_AVERAGED

if [ -e "${OUTPUT}/${FLAG_SUMMARY_AVERAGE_FILE}" ]; then
    if [ "${DO_IT}" == "true" ]; then
	echo "Flag Summary File ${FLAG_SUMMARY_AVERAGE_FILE} for beam $BEAM averaged ms exists - not redoing"
        DO_IT=false
    fi
fi


if [ "${DO_IT}" == "true" ] && [ ! -e "$FLAG_SUMMARY_AVERAGE_CHECK_FILE" ]; then
        setJob flagSummaryAve flgSummAv
    cat > "$sbatchfile" <<EOF
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=01:00:00
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-flagSummaryAverage-b${BEAM}-%j.out"

${askapsoftModuleCommands}
module load bptool

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

log="${logs}/flagSummaryAverage-b${BEAM}_\${SLURM_JOB_ID}.log"
STARTTIME=\$(date +%FT%T)
NCORES=1
NPPN=1
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" flagSummary.py -i ${msSciAvFull} -o ${FLAG_SUMMARY_AVERAGE_FILE} 
err=\$?
echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} "flagSumaryAve" "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
#else
#    # Copy the log from the valiation to the diagnostics directory 
#    cp \${log} \${diagnostics}
fi


EOF
    if [ "${SUBMIT_JOBS}" == "true" ]; then
        DEP=${CONT_PREIMAGE_DEPS}
        ID_FLAG_SUMMARY_AVE=$(sbatch ${DEP} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_FLAG_SUMMARY_AVE}" "Running flagSummary on averaged Science data for beam-${BEAM}, with flags \"$DEP\""
    else
        echo "Would run flagSummary on averaged Science data for beam-${BEAM} with slurm file $sbatchfile"
    fi
    echo " "


fi

