#!/bin/bash -l
#
# Launches a job to create thumbnail images of all 2D FITS
# images. These are done sequentially. For each image, cimstat is used
# to determine the noise (via robust statistics), and the greyscale is
# set to -10 to +40 times the effective rms.
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

if [ "${DO_MAKE_THUMBNAILS}" == "true" ]; then

    sbatchfile="$slurms/makeThumbnails.sbatch"
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
${SLURM_CONFIG}
#SBATCH --time=${JOB_TIME_THUMBNAILS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=thumbnails
${exportDirective}
#SBATCH --output="$slurmOut/slurm-thumbnails-%j.out"

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile="$sbatchfile"
cp "\$thisfile" "\$(echo "\$thisfile" | sed -e "\$sedstr")"


# Define the lists of image names, types, 
. "${getArtifacts}"

log="${logs}/thumbnails_\${SLURM_JOB_ID}.log"
parset="${parsets}/thumbnails_\${SLURM_JOB_ID}.in"

for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do

    cat >> "\$parset" <<EOF
###### Image #\${i} #############
makeThumbnail.image = \${casdaTwoDimImageNames[i]}
makeThumbnail.imageTitle = \${casdaTwoDimThumbTitles[i]}
makeThumbnail.imageSuffix = ${THUMBNAIL_SUFFIX}
makeThumbnail.zmin = ${THUMBNAIL_GREYSCALE_MIN}
makeThumbnail.zmax = ${THUMBNAIL_GREYSCALE_MAX}
makeThumbnail.imageSizes = [${THUMBNAIL_SIZE_INCHES}]
makeThumbnail.imageSizeNames = [${THUMBNAIL_SIZE_TEXT}]
EOF
    
    NCORES=1
    NPPN=1
    srun --export=ALL --ntasks=\${NCORES} --ntasks-per-node=\${NPPN} ${makeThumbnails} -c "\${parset}" >> "\${log}"
    err=\$?
    if [ \$err != 0 ]; then
        echo "ERROR - Thumbnail creation failed for image \${casdaTwoDimImageNames[i]}" | tee -a "\${log}"
        exit \$err
    fi

done

EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterany:$(echo "${ALL_JOB_IDS}" | sed -e 's/,/:/g')"
        fi
        ID_THUMBS=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_THUMBS}" "Job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with flags \"${dep}\""
    else
        echo "Would submit job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with slurm file $sbatchfile"
    fi



fi
