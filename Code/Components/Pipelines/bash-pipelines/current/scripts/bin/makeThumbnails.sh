#!/bin/bash -l
#
# Launches a job to create thumbnail images of all 2D FITS
# images. These are done sequentially. For each image, cimstat is used
# to determine the noise (via robust statistics), and the greyscale is
# set to -10 to +40 times the effective rms.
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

if [ ${DO_MAKE_THUMBNAILS} == true ]; then

    sbatchfile=$slurms/makeThumbnails.sbatch
    cat > $sbatchfile <<EOFOUTER
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
cp $sbatchfile \`echo $sbatchfile | sed -e \$sedstr\`


# Define the lists of image names, types, 
. ${getArtifacts}

for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do

    im=\${casdaTwoDimImageNames[i]}
    title=\${casdaTwoDimThumbTitles[i]}

    StatParset=$parsets/cimstat-\${im}_\${SLURM_JOB_ID}.in
    StatLog=$logs/cimstat-\${im}_\${SLURM_JOB_ID}.log
    cat > \$StatParset <<EOF
Cimstat.image = \${im}
Cimstat.stats = ["Mean","Stddev","Median","MADFM","MADFMasStdDev"]
EOF
    aprun -n 1 $cimstat -c \$StatParset > \$StatLog
    measuredNoise=\`grep "MADFMasStddev" \$StatLog | awk '{print \$10}'\`
    zmin=\`echo \$measuredNoise ${THUMBNAIL_GREYSCALE_MIN} | awk '{printf "%.6f",\$1*\$2}'\`
    zmax=\`echo \$measuredNoise ${THUMBNAIL_GREYSCALE_MAX} | awk '{printf "%.6f",\$1*\$2}'\`

    log=${logs}/thumbnails-\${im}_\${SLURM_JOB_ID}.log
    script=${parsets}/thumbnails-\${im}_\${SLURM_JOBID}.py
    cat > \$script <<EOF
#!/usr/bin/env python
import matplotlib
matplotlib.use('Agg')
matplotlib.rcParams['font.family'] = 'serif'
matplotlib.rcParams['font.serif'] = ['Times', 'Palatino', 'New Century Schoolbook', 'Bookman', 'Computer Modern Roman']
#matplotlib.rcParams['text.usetex'] = True
import aplpy
import pylab as plt
suffix='${THUMBNAIL_SUFFIX}'
fitsim='\${im}'
thumbim=fitsim.replace('.fits','.%s'%suffix)
figtitle='\${title}'
figsizes={'${THUMBNAIL_SIZE_TEXT[0]}':${THUMBNAIL_SIZE_INCHES[0]}, '${THUMBNAIL_SIZE_TEXT[1]}':${THUMBNAIL_SIZE_INCHES[1]}}
for size in figsizes:
    gc=aplpy.FITSFigure(fitsim,figsize=(figsizes[size],figsizes[size]))
    gc.show_colorscale(vmin=\$zmin,vmax=\$zmax)
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
    python \$script > \$log

done
EOFOUTER

    if [ $SUBMIT_JOBS == true ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:`echo $ALL_JOB_IDS | sed -e 's/,/:/g'`"
        fi
        ID_THUMBS=`sbatch ${dep} $sbatchfile | awk '{print $4}'`
        recordJob ${ID_THUMBS} "Job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with flags \"${dep}\""
    else
        echo "Would submit job to create ${THUMBNAIL_SUFFIX} thumbnails of all 2D images, with slurm file $sbatchfile"
    fi



fi
