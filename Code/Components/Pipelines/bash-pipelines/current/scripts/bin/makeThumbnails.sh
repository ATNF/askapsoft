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

    sbatchfile=$slurms/makeThumbnails.sbatch
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_THUMBNAILS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=thumbnails
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-thumbnails-%j.out

${askapsoftModuleCommands}

BASEDIR=${BASEDIR}
cd $OUTPUT
. ${PIPELINEDIR}/utils.sh

# Make a copy of this sbatch file for posterity
sedstr="s/sbatch/\${SLURM_JOB_ID}\.sbatch/g"
thisfile=$sbatchfile
cp \$thisfile "\$(echo \$thisfile | sed -e "\$sedstr")"


# Define the lists of image names, types, 
ADD_FITS_SUFFIX=true
. "${getArtifacts}"

pathToScript=\$(which makeThumbnailImage.py 2> "${tmp}/whchmkthumb")
if [ "\${pathToScript}" == "" ]; then

    for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do
    
        im=\${casdaTwoDimImageNames[i]}
        title=\${casdaTwoDimThumbTitles[i]}
    
        log=${logs}/thumbnails-\${im##*/}_\${SLURM_JOB_ID}.log
        script=${parsets}/thumbnails-\${im##*/}_\${SLURM_JOBID}.py
        cat > "\$script" <<EOF
#!/usr/bin/env python
import matplotlib
matplotlib.use('Agg')
matplotlib.rcParams['font.family'] = 'serif'
matplotlib.rcParams['font.serif'] = ['Times', 'Palatino', 'New Century Schoolbook', 'Bookman', 'Computer Modern Roman']
#matplotlib.rcParams['text.usetex'] = True
import aplpy
import pylab as plt
import pyfits as fits
import os
import numpy as np

# Get statistics for the image
#  If there is a matching weights image, use that to
#  avoid pixels that have zero weight.
fitsim='\${im}'
weightsim=fitsim
weightsim.replace('.restored','')
prefix=weightsim[:weightsim.find('.')]
weightsim.replace(prefix,'weights',1)
image=fits.getdata(fitsim)
isgood=(np.ones(image.shape)>0)
if os.access(weightsim,os.F_OK):
    weights=fits.getdata(weightsim)
    isgood=(weights>0)
median=np.median(image[isgood])
madfm=np.median(abs(image[isgood]-median))
stddev=madfm * 0.6744888
vmin=${THUMBNAIL_GREYSCALE_MIN} * stddev
vmax=${THUMBNAIL_GREYSCALE_MAX} * stddev

suffix='${THUMBNAIL_SUFFIX}'
thumbim=fitsim.replace('.fits','.%s'%suffix)
figtitle='\${title}'
figsizes={'${THUMBNAIL_SIZE_TEXT[0]}':${THUMBNAIL_SIZE_INCHES[0]}, '${THUMBNAIL_SIZE_TEXT[1]}':${THUMBNAIL_SIZE_INCHES[1]}}
for size in figsizes:
    gc=aplpy.FITSFigure(fitsim,figsize=(figsizes[size],figsizes[size]))
    gc.show_colorscale(vmin=vmin,vmax=vmax)
    gc.tick_labels.set_xformat('hh:mm')
    gc.tick_labels.set_yformat('dd:mm')
    gc.add_grid()
    gc.grid.set_linewidth(0.5)
    gc.grid.set_alpha(0.5)
    plt.title(figtitle)
    #
    gc.set_theme('publication')
    gc.save(thumbim.replace('.%s'%suffix,'_%s.%s'%(size,suffix)))

EOF
        python "\$script" > "\$log"
    
    done

else

    log=${logs}/thumbnails_\${SLURM_JOB_ID}.log
    parset=${parsets}/thumbnails_\${SLURM_JOB_ID}.in
    
    for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do
    
        cat >> "\$parset" <<EOF
###### Image #\${i} #############
makeThumbnail.image = \${casdaTwoDimImageNames[i]}
makeThumbnail.imageTitle = \${casdaTwoDimThumbTitles[i]}
makeThumbnail.imageSuffix = ${THUMBNAIL_SUFFIX}
makeThumbnail.zmin = ${THUMBNAIL_GREYSCALE_MIN}
makeThumbnail.zmax = ${THUMBNAIL_GREYSCALE_MAX}
makeThumbnail.imageSizes = ${THUMBNAIL_SIZE_INCHES}
makeThumbnail.imageSizeNames = ${THUMBNAIL_SIZE_TEXT}
EOF
    
        NCORES=1
        NPPN=1
        aprun -n \${NCORES} -N \${NPPN} ${makeThumbnails} -c "\${parset}" >> "\${log}"
        err=\$?
        if [ \$err != 0 ]; then
            echo "ERROR - Thumbnail creation failed for image \${casdaTwoDimImageNames[i]}" | tee -a "\${log}"
            exit \$err
        fi
    
    done

fi
EOFOUTER

    if [ "${SUBMIT_JOBS}" == "true" ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:$(echo "${ALL_JOB_IDS}" | sed -e 's/,/:/g')"
        fi
        ID_THUMBS=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_THUMBS}" "Job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with flags \"${dep}\""
    else
        echo "Would submit job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with slurm file $sbatchfile"
    fi



fi
