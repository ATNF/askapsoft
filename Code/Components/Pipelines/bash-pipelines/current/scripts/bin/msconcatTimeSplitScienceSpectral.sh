#!/bin/bash -l
#
# Launches a job to concatenate the timeWise split high resolution (spectral) 
# science measurement sets 
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

ID_MSCONCAT_SCI_SPECTRAL=""

if [ "$DO_SPECTRAL_IMAGING" == "true" ]; then
	if [ "$DO_SPLIT_TIMEWISE" == "true" ]; then
		DO_IT=true
	else
		DO_IT=false
	fi
fi
 
if [ "$DO_IT" == "true" ] && [ -e "$MSCONCAT_SCI_SPECTRAL_CHECK_FILE" ]; then
    echo "The TimeWise split spectral msdata for beam $BEAM of the science observation has already been concatenated - not re-doing"
    DO_IT=false
fi

if [ "$DO_IT" == "true" ]; then

    setJob msconcat_spectralSci msconcatSPSCI
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_MSCONCAT_SCI_SPECTRAL}
#SBATCH --ntasks=${NUM_CORES_MSCONCAT_SCI_SPECTRAL}
#SBATCH --ntasks-per-node=${NPPN_MSCONCAT_SCI_SPECTRAL}
#SBATCH --job-name=${jobname}
${exportDirective}
#SBATCH --output="$slurmOut/slurm-msconcatSciSpectral-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"

log=${logs}/msconcat_SciSpectral_${FIELDBEAM}_\${SLURM_JOB_ID}.log

STARTTIME=\$(date +%FT%T)
NCORES=${NUM_CORES_MSCONCAT_SCI_SPECTRAL}
NPPN=${NPPN_MSCONCAT_SCI_SPECTRAL}
srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} /usr/bin/time -p -o "\${log}.timing" ${msconcat} -o $msconcatFile $inputs2MSconcatSL > "\$log"
err=\$?
rejuvenate ${msconcatFile}
echo "STARTTIME=\${STARTTIME}" >> "\${log}.timing"
extractStatsNonStandard "\${log}" \${NCORES} "\${SLURM_JOB_ID}" \${err} ${jobname} "txt,csv"
if [ \$err != 0 ]; then
    exit \$err
else
    touch "$MSCONCAT_SCI_SPECTRAL_CHECK_FILE"
fi

# Remove interim MSs if required.
purgeMSs=${PURGE_INTERIM_MS_SCI}
if [ "\${purgeMSs}" == "true" ]; then
    for ms in ${inputs2MSconcatSL}; do
        lfs find \$ms -type f -print0 | xargs -0 munlink
        find \$ms -type l -delete
        find \$ms -depth -type d -empty -delete
    done
fi

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
	DEP=""
        DEP=$(addDep "$DEP" "$DEP_START")
	DEP=$(addDep "$DEP" "$ID_SPLIT_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CCALAPPLY_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_AVERAGE_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_FLAG_SCI_AV_LIST")
        DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI")
        DEP=$(addDep "$DEP" "$ID_CONTIMG_SCI_SC")
        DEP=$(addDep "$DEP" "$ID_SPLIT_SL_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CAL_APPLY_SL_SCI_LIST")
        DEP=$(addDep "$DEP" "$ID_CONT_SUB_SL_SCI_LIST")


	ID_MSCONCAT_SCI_SPECTRAL=$(sbatch $DEP "$sbatchfile" | awk '{print $4}')
	recordJob "${ID_MSCONCAT_SCI_SPECTRAL}" "MS-Concatenating spectral science observation, with flags \"$DEP\""
    else
	echo "Would run msconcat on spectral science observation with slurm file $sbatchfile"
    fi

    echo " "

fi
