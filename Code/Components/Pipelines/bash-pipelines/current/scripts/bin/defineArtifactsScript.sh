#!/bin/bash -l
#
# Creates a local script that can be used to get the artifacts needed
# for archiving. This local script is needed as we need to know which
# images are present when we actually execute various jobs (such as
# the thumbnail creation & the casdaupload), rather than when we run
# processBETA.
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

# Define list of possible MSs:
msNameList=()
for BEAM in ${BEAMS_TO_USE}; do
    findScienceMSnames
    if [ $DO_APPLY_CAL_CONT == true ]; then
        msNameList+=($msSciAvCal)
    else
        msNameList+=($msSciAv)
    fi
done



getArtifacts=${tools}/getArchiveList-${NOW}.sh
cat > ${getArtifacts} <<EOF
#!/bin/bash -l
#
# Defines the lists of images, catalogues and measurement sets that
# are present and need to be archived. Upon return, the following
# environment variables are defined:
#   * casdaTwoDimImageNames - list of 2D FITS files to be archived
#   * casdaTwoDimImageTypes - their corresponding image types
#   * casdaTwoDimThumbTitles - the titles used in the thumbnail images
#       of the 2D images
#   * casdaOtherDimImageNames - list of non-2D FITS files to be archived
#   * casdaOtherDimImageTypes - their corresponding image types
#   * catNames - names of catalogue files to archived
#   * catTypes - their corresponding catalogue types
#   * msNames - names of measurement sets to be archived
#   * evalNames - names of evaluation files to be archived
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

BASEDIR=${BASEDIR}
parsets=$parsets
logs=$logs

utilsScript=$PIPELINEDIR/utils.sh
. \$utilsScript

# List of images and their types

##############################
# First, search for continuum images
casdaTwoDimImageNames=()
casdaTwoDimImageTypes=()
casdaTwoDimThumbTitles=()
casdaOtherDimImageNames=()
casdaOtherDimImageTypes=()

# Variables defined from configuration file
NOW="${NOW}"
nterms="${NUM_TAYLOR_TERMS}"
list_of_images="${IMAGE_LIST}"
doBeams="${ARCHIVE_BEAM_IMAGES}"
beams="${BEAMS_TO_USE}"
FIELD_LIST="${FIELD_LIST}"
IMAGE_BASE_CONT="${IMAGE_BASE_CONT}"
IMAGE_BASE_CONTCUBE="${IMAGE_BASE_CONTCUBE}"
IMAGE_BASE_SPECTRAL="${IMAGE_BASE_SPECTRAL}"

LOCAL_BEAM_LIST="all"
if [ "\${doBeams}" == true ]; then
    LOCAL_BEAM_LIST="\$LOCAL_BEAM_LIST \${beams}"
fi

for FIELD in \${FIELD_LIST}; do

    for BEAM in \${LOCAL_BEAM_LIST}; do
    
        # call this to get the image base name \${imageBase} for continuum images, getting linmos in there correctly
        setImageBaseCont
        
        # Gather lists of continuum images
    
        if [ \${BEAM} == "all" ]; then
            beamSuffix="mosaic"
        else
            beamSuffix="beam \${BEAM}"
        fi
        
        for((i=0;i<\${nterms};i++)); do
    
            if [ \${nterms} -eq 1 ]; then
                imBase=\${imageBase}
            else
                imBase="\${imageBase}.taylor.\${i}"
            fi
            typeSuffix="T\${i}"
    
            # The following makes lists of the FITS images suitable for CASDA, and their associated image types
            if [ -e \${FIELD}/image.\${imBase}.restored.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/image.\${imBase}.restored.fits)
                casdaTwoDimImageTypes+=(cont_restored_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Restored image, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/image.\${imBase}.alt.restored.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/image.\${imBase}.restored.fits)
                casdaTwoDimImageTypes+=(cont_restored_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Restored image, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/psf.\${imBase}.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/psf.\${imBase}.fits)
                casdaTwoDimImageTypes+=(cont_psfnat_\${typeSuffix})
                casdaTwoDimThumbTitles+=("PSF, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/psf.image.\${imBase}.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/psf.image.\${imBase}.fits)
                casdaTwoDimImageTypes+=(cont_psfprecon_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Preconditioned PSF, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/image.\${imBase}.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/image.\${imBase}.fits)
                casdaTwoDimImageTypes+=(cont_cleanmodel_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Clean model, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/residual.\${imBase}.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/residual.\${imBase}.fits)
                casdaTwoDimImageTypes+=(cont_residual_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Residual image, \${beamSuffix}")
            fi
            if [ -e \${FIELD}/sensitivity.\${imBase}.fits ]; then
                casdaTwoDimImageNames+=(\${FIELD}/sensitivity.\${imBase}.fits)
                casdaTwoDimImageTypes+=(cont_sensitivity_\${typeSuffix})
                casdaTwoDimThumbTitles+=("Sensitivity, \${beamSuffix}")
            fi
            
        done
    
        # Gather lists of spectral cubes
    
        # set the imageBase variable for the spectral-line case
        setImageBaseSpectral
        # The following makes lists of the FITS images suitable for CASDA, and their associated image types
        if [ -e \${FIELD}/image.\${imageBase}.restored.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/image.\${imageBase}.restored.fits)
            casdaOtherDimImageTypes+=(spectral_restored_3d)
        fi
        if [ -e \${FIELD}/psf.\${imageBase}.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/psf.\${imageBase}.fits)
            casdaOtherDimImageTypes+=(spectral_psfnat_3d)
        fi
        if [ -e \${FIELD}/psf.image.\${imageBase}.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/psf.image.\${imageBase}.fits)
            casdaOtherDimImageTypes+=(spectral_psfprecon_3d)
        fi
        if [ -e \${FIELD}/image.\${imageBase}.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/image.\${imageBase}.fits)
            casdaOtherDimImageTypes+=(spectral_cleanmodel_3d)
        fi
        if [ -e \${FIELD}/residual.\${imageBase}.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/residual.\${imageBase}.fits)
            casdaOtherDimImageTypes+=(spectral_residual_3d)
        fi
        if [ -e \${FIELD}/sensitivity.\${imageBase}.fits ]; then
            casdaOtherDimImageNames+=(\${FIELD}/sensitivity.\${imageBase}.fits)
            casdaOtherDimImageTypes+=(spectral_sensitivity_3d)
        fi
    
    done

done

##############################
# Next, search for catalogues

# Only continuum catalogues so far - both islands and components

catNames=()
catTypes=()

BEAM=all
for FIELD in \${FIELD_LIST}; do

    setImageBaseCont
    contSelDir=selavy_\${imageBase}
    if [ -e \${FIELD}/\${contSelDir}/selavy-results.components.xml ]; then
        catNames+=(\${FIELD}/\${contSelDir}/selavy-results.components.xml)
        catTypes+=(continuum-component)
    fi
    if [ -e \${FIELD}/\${contSelDir}/selavy-results.islands.xml ]; then
        catNames+=(\${FIELD}/\${contSelDir}/selavy-results.islands.xml)
        catTypes+=(continuum-island)
    fi

done
##############################
# Next, search for MSs

# for now, get all averaged continuum beam-wise MSs
msNames=()
possibleMSnames="${msNameList[@]}"

for FIELD in \${FIELD_LIST}; do

    for ms in \${possibleMSnames}; do
    
        if [ -e \${FIELD}/\${ms} ]; then
            msNames+=(\${FIELD}/\${ms})
        fi
    
    done

done

##############################
# Next, search for evaluation-related documents

evalNames=()

# Stats summary files
for file in \`\ls ${OUTPUT}/stats-all*.txt\`; do
    evalNames+=(\${file##*/})
done

EOF
