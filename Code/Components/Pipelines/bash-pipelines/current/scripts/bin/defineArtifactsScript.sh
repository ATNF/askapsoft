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
    msNameList+=($msSciAv)
done



getArtifacts=${tools}/getArchiveList-${NOW}.sh
cat > ${getArtifacts} <<EOF
#!/bin/bash -l
#
# Defines the lists of images, catalogues and measurement sets that
# are present and need to be archived. Upon return, the following
# environment variables are defined:
#   * casdaImageNames - list of FITS files to be archived
#   * casdaImageTypes - their corresponding image types
#   * imageNames - list of all CASA-format images
#   * imageNamesTwoDim - list of only the 2D CASA-format images
#   * thumbnailTitles - the titles used in the thumbnail images of the
#     2D images
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

# Run the config file
. ${PIPELINEDIR}/defaultConfig.sh
. ${archivedConfig}
. ${PIPELINEDIR}/processDefaults.sh


# List of images and their types

##############################
# First, search for continuum images
casdaImageNames=()
casdaImageTypes=()
imageNames=()
imageNamesTwoDim=()
thumbnailTitles=()

# Variables defined from configuration file
NOW="${NOW}"
nterms="${NUM_TAYLOR_TERMS}"
list_of_images="${IMAGE_LIST}"
doBeams="${ARCHIVE_BEAM_IMAGES}"
beams="${BEAMS_TO_USE}"

LOCAL_BEAM_LIST="all"
if [ "\${doBeams}" == true ]; then
    LOCAL_BEAM_LIST="\$LOCAL_BEAM_LIST \${beams}"
fi


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
        if [ -e image.\${imBase}.restored.fits ]; then
            casdaImageNames+=(image.\${imBase}.restored.fits)
            casdaImageTypes+=(cont_restored_\${typeSuffix})
            imageNamesTwoDim+=(image.\${imBase}.restored.fits)
            thumbnailTitles+=("Restored image, \${beamSuffix}")
        fi
        if [ -e psf.\${imBase}.fits ]; then
            casdaImageNames+=(psf.\${imBase}.fits)
            imageNamesTwoDim+=(psf.\${imBase}.fits)
            casdaImageTypes+=(cont_psfnat_\${typeSuffix})
            thumbnailTitles+=("PSF, \${beamSuffix}")
        fi
        if [ -e psf.image.\${imBase}.fits ]; then
            casdaImageNames+=(psf.image.\${imBase}.fits)
            imageNamesTwoDim+=(psf.image.\${imBase}.fits)
            casdaImageTypes+=(cont_psfprecon_\${typeSuffix})
            thumbnailTitles+=("Preconditioned PSF, \${beamSuffix}")
        fi
        if [ -e image.\${imBase}.fits ]; then
            casdaImageNames+=(image.\${imBase}.fits)
            imageNamesTwoDim+=(image.\${imBase}.fits)
            casdaImageTypes+=(cont_cleanmodel_\${typeSuffix})
            thumbnailTitles+=("Clean model, \${beamSuffix}")
        fi
        if [ -e residual.\${imBase}.fits ]; then
            casdaImageNames+=(residual.\${imBase}.fits)
            imageNamesTwoDim+=(residual.\${imBase}.fits)
            casdaImageTypes+=(cont_residual_\${typeSuffix})
            thumbnailTitles+=("Residual image, \${beamSuffix}")
        fi
        if [ -e sensitivity.\${imBase}.fits ]; then
            casdaImageNames+=(sensitivity.\${imBase}.fits)
            imageNamesTwoDim+=(sensitivity.\${imBase}.fits)
            casdaImageTypes+=(cont_sensitivity_\${typeSuffix})
            thumbnailTitles+=("Sensitivity, \${beamSuffix}")
        fi

        # The following makes a list of CASA images
        for im in \${list_of_images}; do

            for imagename in "\${im}.\${imBase}" "\${im}.\${imBase}.restored"; do
            
                if [ -e \${imagename} ]; then
                echo \${imagename}
                    imageNames+=(\${imagename})
                fi

            done

        done                
        
    done

    # Gather lists of spectral cubes

    # set the imageBase variable for the spectral-line case
    setImageBaseSpectral
    # The following makes lists of the FITS images suitable for CASDA, and their associated image types
    if [ -e image.\${imageBase}.restored.fits ]; then
        casdaImageNames+=(image.\${imageBase}.restored.fits)
        casdaImageTypes+=(spectral_restored_3d)
    fi
    if [ -e psf.\${imageBase}.fits ]; then
        casdaImageNames+=(psf.\${imageBase}.fits)
        casdaImageTypes+=(spectral_psfnat_3d)
    fi
    if [ -e psf.image.\${imageBase}.fits ]; then
        casdaImageNames+=(psf.image.\${imageBase}.fits)
        casdaImageTypes+=(spectral_psfprecon_3d)
    fi
    if [ -e image.\${imageBase}.fits ]; then
        casdaImageNames+=(image.\${imageBase}.fits)
        casdaImageTypes+=(spectral_cleanmodel_3d)
    fi
    if [ -e residual.\${imageBase}.fits ]; then
        casdaImageNames+=(residual.\${imageBase}.fits)
        casdaImageTypes+=(spectral_residual_3d)
    fi
    if [ -e sensitivity.\${imageBase}.fits ]; then
        casdaImageNames+=(sensitivity.\${imageBase}.fits)
        casdaImageTypes+=(spectral_sensitivity_3d)
    fi
    
    # The following makes lists of CASA images
    for im in \${list_of_images}; do
        
        for imagename in "\${im}.\${imBase}" "\${im}.\${imBase}.restored"; do
            
            if [ -e \${imagename} ]; then
                imageNames+=(\${imagename})
            fi

        done
        
    done                


done

##############################
# Next, search for catalogues

# Only continuum catalogues so far - both islands and components

catNames=()
catTypes=()

BEAM=all
setImageBaseCont
contSelDir=selavy_\${imageBase}
if [ -e \${contSelDir}/selavy-results.components.xml ]; then
    catNames+=(\${contSelDir}/selavy-results.components.xml)
    catTypes+=(continuum-component)
fi
if [ -e \${contSelDir}/selavy-results.islands.xml ]; then
    catNames+=(\${contSelDir}/selavy-results.islands.xml)
    catTypes+=(continuum-island)
fi

##############################
# Next, search for MSs

# for now, get all averaged continuum beam-wise MSs
msNames=()
possibleMSnames="${msNameList[@]}"

for ms in \${possibleMSnames}; do

    if [ -e \$ms ]; then
        msNames+=(\$ms)
    fi

done

##############################
# Next, search for evaluation-related documents

evalNames=()

# Stats summary files
for file in \`\ls ${OUTPUT}/stats-all*.txt\`; do
    evalNames+=(\${file})
done

# Thumbnail images
suffix=${THUMBNAIL_SUFFIX}
sizelist="${THUMBNAIL_SIZES_INCHES}"
for size in \`echo \$sizelist | sed -e "s/[{}\,\']//g"\`; do 
    sizeStr=\`echo \$size | awk -F':' '{print \$1}'\`
    sedStr="s/\.fits/_\${sizeStr}\.\${suffix}/g"
    for((i=0;i<\${#imageNamesTwoDim[@]};i++)); do
        thumb=\`echo \${imageNamesTwoDim[i]} | sed -e \$sedStr\`
        if [ -e \$thumb ]; then
            evalNames+=(\${thumb})
        fi
    done
done

EOF
