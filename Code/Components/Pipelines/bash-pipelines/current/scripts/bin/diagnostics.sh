#!/bin/bash -l
#
# A script to run diagnostic tasks on the data products that have been
# created, producing plots and other files used for QA and related
# purposes. 
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

DO_IT=$DO_DIAGNOSTICS

if [ "${DO_IT}" == "true" ]; then

    sbatchfile=$slurms/runDiagnostics.sbatch
    cat > "$sbatchfile" <<EOFOUTER
#!/bin/bash -l
#SBATCH --partition=${QUEUE}
#SBATCH --clusters=${CLUSTER}
${ACCOUNT_REQUEST}
${RESERVATION_REQUEST}
#SBATCH --time=${JOB_TIME_DIAGNOSTICS}
#SBATCH --ntasks=1
#SBATCH --ntasks-per-node=1
#SBATCH --job-name=diagnostics
${EMAIL_REQUEST}
${exportDirective}
#SBATCH --output=$slurmOut/slurm-diagnostics-%j.out

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

# Make PNG images of continuum images, overlaid with weights contours
# and fitted components

log=${logs}/diagnostics_\${SLURM_JOB_ID}.log
diagParset=${parsets}/diagnostics_\${SLURM_JOB_ID}.in    

pathToScript=\$(which makeThumbnailImage.py 2> "${tmp}/whchmkthumb")
if [ "\${pathToScript}" != "" ]; then

    for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do
    
        if [ "\${casdaTwoDimImageTypes[i]}" == "cont_restored_T0" ]; then

            imdir="\${casdaTwoDimImageNames[i]%/*}"
            imageNoFITS=\$(echo "\${casdaTwoDimImageNames[i]##*/}"| sed -e 's/\\.fits//g')
            echo "image = \${imageNoFITS}"

            # make a second plot showing catalogued source positions
            seldir="\${imdir}/selavy_\${imageNoFITS}"
            echo "selavy dir = \${seldir}"
            catalogue="\${seldir}/selavy-\${imageNoFITS}.components.txt"
            echo "catalogue = \$catalogue"
            if [ -e "\${seldir}" ] && [ -e "\${catalogue}" ]; then

                cat >> "\$diagParset" <<EOF
###### Image #\${i} catalogue #############
makeThumbnail.image = \${casdaTwoDimImageNames[i]}
makeThumbnail.imageTitle = \${casdaTwoDimThumbTitles[i]}
makeThumbnail.catalogue = \${catalogue}
makeThumbnail.outdir = ${diagnostics}
makeThumbnail.imageSuffix = ${THUMBNAIL_SUFFIX}
makeThumbnail.zmin = ${THUMBNAIL_GREYSCALE_MIN}
makeThumbnail.zmax = ${THUMBNAIL_GREYSCALE_MAX}
makeThumbnail.imageSizes = [16]
makeThumbnail.imageSizeNames = [sources]
makeThumbnail.showWeightsContours = True
EOF
    
                NCORES=1
                NPPN=1
                aprun -n \${NCORES} -N \${NPPN} ${makeThumbnails} -c "\${diagParset}" >> "\${log}"
                err=\$?
                if [ \$err != 0 ]; then
                    echo "ERROR - Sources thumbnail creation failed for image \${casdaTwoDimImageNames[i]}" | tee -a "\${log}"
                    exit \$err
                fi

            fi

        fi

    done

else

    echo "No image thumbnails with catalogues produced, since $makeThumbnails doesn't exist in this module"

fi

#####################

# Make thumbnail images of the noise maps created by Selavy

if [ "\${pathToScript}" != "" ]; then

    fitsParset=${parsets}/diagnostics_fitsConversion_\${SLURM_JOB_ID}.

    for((i=0;i<\${#casdaTwoDimImageNames[@]};i++)); do

        imdir="\${casdaTwoDimImageNames[i]%/*}"
        imageNoFITS=\$(echo "\${casdaTwoDimImageNames[i]##*/}"| sed -e 's/\\.fits//g')
        echo "image = \$imageNoFITS"
    
        if [ "\${casdaTwoDimImageTypes[i]}" == "cont_restored_T0" ]; then

            # make a second plot showing catalogued source positions
            seldir="\${imdir}/selavy_\${imageNoFITS}"
            noisemapbase="\${seldir}/noiseMap.\${imageNoFITS}"
            echo "selavy dir = \$seldir"
            echo "noise map base = \$noisemapbase"

            if [ ! -e "\${noisemapbase}.fits" ] && [ -e "\${noisemapbase}.img" ]; then
                # need to convert to FITS
                parset=\${fitsParset}
                casaim=\${noisemapbase}.img
                fitsim=\${noisemapbase}.fits
                ${fitsConvertText}
            fi
            catalogue="\${seldir}/selavy-\${imageNoFITS}.components.txt"
            if [ -e "\${noisemapbase}.fits" ]; then
                cat >> "\$diagParset" <<EOF
###### Image #\${i} catalogue #############
makeThumbnail.image = \${noisemapbase}.fits
makeThumbnail.weights = \${seldir}/\$(echo \$imageNoFITS | sed -e 's/image\./weights\./g' | sed -e 's/\.restored//g').fits
makeThumbnail.imageTitle = \${casdaTwoDimThumbTitles[i]} - noise map
makeThumbnail.catalogue = \${catalogue}
makeThumbnail.outdir = ${diagnostics}
makeThumbnail.imageSuffix = ${THUMBNAIL_SUFFIX}
makeThumbnail.zmin = 0
makeThumbnail.zmax = 10
makeThumbnail.imageSizes = [16]
makeThumbnail.imageSizeNames = [sources]
makeThumbnail.showWeightsContours = True
EOF
    
                NCORES=1
                NPPN=1
                aprun -n \${NCORES} -N \${NPPN} ${makeThumbnails} -c "\${diagParset}" >> "\${log}"
                err=\$?
                if [ \$err != 0 ]; then
                    echo "ERROR - Sources thumbnail creation failed for image \${noisemapbase}.fits" | tee -a "\${log}"
                    exit \$err
                fi

            fi

        fi

    done

else

    echo "No image thumbnails with catalogues produced, since $makeThumbnails doesn't exist in this module"

fi

EOFOUTER

        if [ "${SUBMIT_JOBS}" == "true" ]; then
        dep=""
        if [ "${ALL_JOB_IDS}" != "" ]; then
            dep="-d afterok:$(echo "${ALL_JOB_IDS}" | sed -e 's/,/:/g')"
        fi
        ID_DIAG=$(sbatch ${dep} "$sbatchfile" | awk '{print $4}')
        recordJob "${ID_DIAG}" "Job to create diagnostic plots, with flags \"${dep}\""
    else
        echo "Would submit job to create diagnostic plots, with slurm file $sbatchfile"
    fi

fi


