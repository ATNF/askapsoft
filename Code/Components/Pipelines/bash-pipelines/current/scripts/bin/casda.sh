#!/bin/bash -l
#
# Launches a job to build the observation.xml file, and stages data
# ready for ingest into CASDA.
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

if [ $DO_STAGE_FOR_CASDA == true ]; then

    if [ "${OTHER_SBIDS}" != "" ]; then
        sbids="sbids                           = ${OTHER_SBIDS}"
    else
        sbids="# No other sbids provided."
    fi

    if [ ! -w ${CASDA_OUTPUT_DIR} ]; then
        echo "WARNING - desired CASDA output directory ${CASDA_OUTPUT_DIR} is not writeable."
        echo "        - changing output directory to ${OUTPUT}/For-CASDA"
        CASDA_OUTPUT_DIR="${OUTPUT}/For-CASDA"
    fi
    
    sbatchfile=$slurms/casda_upload.sbatch
    cat > $sbatchfile <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_CASDA_UPLOAD}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=casdaupload
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-casdaupload-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh	

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`

# Define the lists of image names, types, 
. ${getArtifacts}

# Set image-related parameters
imageArtifacts=()
imageParams="# Individual image details"
for((i=0;i<\${#casdaImageNames[@]};i++)); do
    imageArtifacts+=(image\${i})
    imageParams="\${imageParams}
image\${i}.filename  = \${casdaImageNames[i]}
image\${i}.type      = \${casdaImageTypes[i]}
image\${i}.project   = ${PROJECT_ID}"
done
imageArtifacts=\`echo \${imageArtifacts[@]} | sed -e 's/ /,/g'\`

catArtifacts=()
catParams="# Individual catalogue details"
for((i=0;i<\${#catNames[@]};i++)); do
    catArtifacts+=(cat\${i})
    catParams="\${catParams}
catalogue\${i}.filename = \${catNames[i]}
catalogue\${i}.type     = \${catTypes[i]}
catalogue\${i}.project  = ${PROJECT_ID}"
done
catArtifacts=\`echo \${catArtifacts[@]} | sed -e 's/ /,/g'\`

msArtifacts=()
msParams="# Individual catalogue details"
for((i=0;i<\${#msNames[@]};i++)); do
    msArtifacts+=(ms\${i})
    msParams="\${msParams}
ms\${i}.filename = \${msNames[i]}
ms\${i}.project  = ${PROJECT_ID}"
done
msArtifacts=\`echo \${msArtifacts[@]} | sed -e 's/ /,/g'\`

evalArtifacts=()
evalParams="# Evaluation file details"
for((i=0;i<\${#evalNames[@]};i++)); do
    evalArtifacts+=(eval\${i})
    evalParams="\${evalParams}
eval\${i}.filename = \${evalNames[i]}
eval\${i}.project  = ${PROJECT_ID}"
done
evalArtifacts=\`echo \${evalArtifacts[@]} | sed -e 's/ /,/g'\`

parset=${parsets}/casda_upload_\${SLURM_JOB_ID}.in
log=${logs}/casda_upload_\${SLURM_JOB_ID}.log
cat > \$parset << EOFINNER
# General
outputdir                       = ${CASDA_OUTPUT_DIR}
telescope                       = ASKAP
sbid                            = ${SB_SCIENCE}
${sbids}
obsprogram                      = ${OBS_PROGRAM}
writeREADYfile                  = ${WRITE_CASDA_READY}

# Images
images.artifactlist             = [\$imageArtifacts]
\${imageParams}

# Source catalogues
catalogues.artifactlist         = [\$catArtifacts]
\${catParams}

# Measurement sets
measurementsets.artifactlist    = [\$msArtifacts]
\${msParams}

# Evaluation reports
evaluation.artifactlist         = [\$evalArtifacts]
\${evalParams}

EOFINNER

NCORES=1
NPPN=1
aprun -n \${NCORES} -N \${NPPN} $casdaupload -c \$parset > \$log
err=\$?
if [ \$err != 0 ]; then
    exit \$err
fi

EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then    
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:`echo $ALL_JOB_IDS | sed -e 's/,/:/g'`"
        fi
        ID_CASDA=`sbatch ${dep} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_CASDA} "Job to stage data for ingest into CASDA, with flags \"${dep}\""
    else
        echo "Would submit job to stage data for ingest into CASDA, with slurm file $sbatchfile"
    fi



fi