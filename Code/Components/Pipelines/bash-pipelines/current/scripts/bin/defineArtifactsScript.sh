#!/bin/bash -l
#
# Creates a local script that can be used to get the artifacts needed
# for archiving. This local script is needed as we need to know which
# images are present when we actually execute various jobs (such as
# the thumbnail creation & the casdaupload), rather than when we run
# processASKAP.
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

# Define list of possible MSs:
#msNameList=()
msNameList=""
for BEAM in ${BEAMS_TO_USE}; do
    IFS="${IFS_FIELDS}"
    for FIELD in ${FIELD_LIST}; do
        findScienceMSnames
        if [ "${DO_APPLY_CAL_CONT}" == "true" ]; then
            #            msNameList+=("${FIELD}/${msSciAvCal}")
            if [ "${msNameList}" == "" ]; then
                msNameList="${FIELD}/${msSciAvCal}"
            else
                msNameList="${msNameList}
${FIELD}/${msSciAvCal}"
            fi
        else
            if [ "${msNameList}" == "" ]; then
                msNameList="${FIELD}/${msSciAv}"
            else
                msNameList="${msNameList}
${FIELD}/${msSciAv}"
            fi
        fi
        if [ "${ARCHIVE_SPECTRAL_MS}" == "true" ]; then
            msNameList="${msNameList}
${FIELD}/${msSci}"
        fi
    done
    IFS="${IFS_DEFAULT}"
done
    
imageCodeList="restored altrestored image contsub residual sensitivity psf psfimage"

getArtifacts="${tools}/getArchiveList-${NOW}.sh"
cat > "${getArtifacts}" <<EOF
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
#   * casdaOtherDimImageSpectra - extracted spectra from 3D cubes
#   * casdaOtherDimImageNoise - extracted noise spectra from 3D cubes
#   * casdaOtherDimImageMoment0s - extracted moment-0 maps
#   * casdaOtherDimImageMoment1s - extracted moment-1 maps
#   * casdaOtherDimImageMoment2s - extracted moment-2 maps
#   * casdaOtherDimImageCubelets - extracted cubelets
#   * casdaOtherDimImageFDF - derived Faraday Dispersion Functions
#   * casdaOtherDimImageRMSF - derived Rotation Measure Spread Functions
#   * casdaOtherDimImagePol - lower-case polarisation character
#   * catNames - names of catalogue files to archived
#   * catTypes - their corresponding catalogue types
#   * msNames - names of measurement sets to be archived
#   * calTables - names of calibration tables to be archived (for now
#       these are included in a tar file sent with the evaluation files)
#   * evalNames - names of evaluation files to be archived
#   * evalFormats - format strings for the evaluation files
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
casdaOtherDimImageSpectra=()
casdaOtherDimImageNoise=()
casdaOtherDimImageMoment0s=()
casdaOtherDimImageMoment1s=()
casdaOtherDimImageMoment2s=()
casdaOtherDimImageCubelets=()
casdaOtherDimImageFDF=()
casdaOtherDimImageRMSF=()
casdaOtherDimImagePol=()

# Variables defined from configuration file or metadata
NOW="${NOW}"
PROJECT_ID="${PROJECT_ID}"
SB_SCIENCE="${SB_SCIENCE}"
archivedConfig="${archivedConfig}"
NUM_TAYLOR_TERMS="${NUM_TAYLOR_TERMS}"
list_of_images="${IMAGE_LIST}"
doBeams="${ARCHIVE_BEAM_IMAGES}"
doSelfcalLoops="${ARCHIVE_SELFCAL_LOOP_MOSAICS}"
doFieldMosaics="${ARCHIVE_FIELD_MOSAICS}"
beams="${BEAMS_TO_USE}"
FIELD_LIST="${FIELD_LIST}"
NUM_FIELDS="${NUM_FIELDS}"
IMAGE_BASE_CONT="${IMAGE_BASE_CONT}"
IMAGE_BASE_CONTCUBE="${IMAGE_BASE_CONTCUBE}"
IMAGE_BASE_SPECTRAL="${IMAGE_BASE_SPECTRAL}"
GAINS_CAL_TABLE="${GAINS_CAL_TABLE}"
POL_LIST="${POL_LIST}"
DO_RM_SYNTHESIS="${DO_RM_SYNTHESIS}"
PREPARE_FOR_CASDA="\${PREPARE_FOR_CASDA}"

DO_ALT_IMAGER_CONT="${DO_ALT_IMAGER_CONT}"
DO_ALT_IMAGER_CONTCUBE="${DO_ALT_IMAGER_CONTCUBE}"
DO_ALT_IMAGER_SPECTRAL="${DO_ALT_IMAGER_SPECTRAL}"
ALT_IMAGER_SINGLE_FILE_CONTCUBE="${ALT_IMAGER_SINGLE_FILE_CONTCUBE}"
ALT_IMAGER_SINGLE_FILE="${ALT_IMAGER_SINGLE_FILE}"

IMAGETYPE_CONT=${IMAGETYPE_CONT}
IMAGETYPE_CONTCUBE=${IMAGETYPE_CONTCUBE}
IMAGETYPE_SPECTRAL=${IMAGETYPE_SPECTRAL}

DO_CONTINUUM_VALIDATION="${DO_CONTINUUM_VALIDATION}"

CONTSUB_METHOD="${CONTSUB_METHOD}"

NUM_LOOPS=0
DO_SELFCAL=$DO_SELFCAL
if [ "\${DO_SELFCAL}" == "true" ] && [ "\$doSelfcalLoops" == "true" ]; then
    NUM_LOOPS=$SELFCAL_NUM_LOOPS
fi

LOCAL_BEAM_LIST="all"
if [ "\${doBeams}" == "true" ]; then
    LOCAL_BEAM_LIST="\$LOCAL_BEAM_LIST \${beams}"
fi

LOCAL_FIELD_LIST="."
if [ "\${doFieldMosaics}" == "true" ]; then
    LOCAL_FIELD_LIST="\$LOCAL_FIELD_LIST
${FIELD_LIST}"
fi

IFS="${IFS_FIELDS}"
for FIELD in \${LOCAL_FIELD_LIST}; do

    if [ "\${FIELD}" == "." ]; then
        theBeamList="all"
        TILE_LIST="${TILE_LIST}
ALL"
    else
        theBeamList=\${LOCAL_BEAM_LIST}
        getTile
        TILE_LIST="\${TILE}"
    fi

    for TILE in \${TILE_LIST}; do

        IFS="${IFS_DEFAULT}"
        for BEAM in \${theBeamList}; do
                
            # Gather lists of continuum images
        
            for imageCode in ${imageCodeList}; do
        
                for((TTERM=0;TTERM<NUM_TAYLOR_TERMS;TTERM++)); do
        
                    for((LOOP=0;LOOP<=NUM_LOOPS;LOOP++)); do

                        if [ \$LOOP -gt 0 ] || [ "\$BEAM" == "all" ]; then

                            setImageProperties cont

                            if [ \$LOOP -gt 0 ]; then
                                if [ "\${IMAGETYPE_CONT}" == "fits" ]; then
                                    imageName="\${imageName%%.fits}.SelfCalLoop\${LOOP}.fits"
                                    weightsImage="\${weightsImage%%.fits}.SelfCalLoop\${LOOP}.fits"
                                else
                                    imageName="\${imageName}.SelfCalLoop\${LOOP}"
                                    weightsImage="\${weightsImage}.SelfCalLoop\${LOOP}"
                                fi
                            fi

                            if [ -e "\${FIELD}/\${imageName}" ]; then
                                casdaTwoDimImageNames+=("\${FIELD}/\${imageName}")
                                casdaTwoDimImageTypes+=("\${imageType}")
                                casdaTwoDimThumbTitles+=("\${label}")
                                if [ "\${BEAM}" == "all" ] && [ "\${imageCode}" == "restored" ]; then
                                    casdaTwoDimImageNames+=("\${FIELD}/\${weightsImage}")
                                    casdaTwoDimImageTypes+=("\${weightsType}")
                                    casdaTwoDimThumbTitles+=("\${weightsLabel}")
                                fi
                            fi

                            if [ \$LOOP -gt 0 ]; then
                                if [ "\${IMAGETYPE_CONT}" == "fits" ]; then
                                    noiseMap="\${noiseMap%%.fits}.SelfCalLoop\${LOOP}.fits"
                                    compMap="\${compMap%%.fits}.SelfCalLoop\${LOOP}.fits"
                                    compResidual="\${compResidual%%.fits}.SelfCalLoop\${LOOP}.fits"
                                else
                                    noiseMap="\${noiseMap%%.fits}.SelfCalLoop\${LOOP}"
                                    compMap="\${compMap%%.fits}.SelfCalLoop\${LOOP}"
                                    compResidual="\${compResidual%%.fits}.SelfCalLoop\${LOOP}"
                                fi
                            fi
                            if [ -e "\${FIELD}/\${selavyDir}/\${noiseMap}" ]; then
                                casdaTwoDimImageNames+=("\${FIELD}/\${selavyDir}/\${noiseMap}")
                                casdaTwoDimImageTypes+=(\${noiseType})
                                casdaTwoDimThumbTitles+=("\${noiseLabel}")
                                casdaTwoDimImageNames+=("\${FIELD}/\${selavyDir}/\${compMap}")
                                casdaTwoDimImageTypes+=(\${compMapType})
                                casdaTwoDimThumbTitles+=("\${compMapLabel}")
                                casdaTwoDimImageNames+=("\${FIELD}/\${selavyDir}/\${compResidual}")
                                casdaTwoDimImageTypes+=(\${compResidualType})
                                casdaTwoDimThumbTitles+=("\${compResidualLabel}")
                            fi

                        fi

                    done
        
                    # Get the continuum models used for continuum subtraction
                    setContsubFilenames
                    if [ "\${CONTSUB_METHOD}" == "CleanModel" ]; then
                        if [ -e "\${FIELD}/\${contsubDir}/\${contsubCleanModelImage}" ]; then
                            casdaTwoDimImageNames+=("\${FIELD}/\${contsubDir}/\${contsubCleanModelImage}")
                            casdaTwoDimImageTypes+=(\${contsubCleanModelType})
                            casdaTwoDimThumbTitles+=("\${contsubCleanModelLabel}")
                        fi
                    fi
                    if [ "\${CONTSUB_METHOD}" == "Cmodel" ]; then
                        if [ -e "\${FIELD}/\${contsubDir}/\${contsubCmodelImage}" ]; then
                            casdaTwoDimImageNames+=("\${FIELD}/\${contsubDir}/\${contsubCmodelImage}")
                            casdaTwoDimImageTypes+=(\${contsubCmodelType})
                            casdaTwoDimThumbTitles+=("\${contsubCmodelLabel}")
                        fi
                    fi

                done
        
            done
        
        
            # Gather lists of spectral cubes & continuum cubes
        
            for imageCode in ${imageCodeList}; do

                for subband in ${SUBBAND_WRITER_LIST}; do
        
                    setImageProperties spectral
                    if [ -e "\${FIELD}/\${imageName}" ]; then
                        casdaOtherDimImageNames+=("\${FIELD}/\${imageName}")
                        casdaOtherDimImageTypes+=("\${imageType}")
                        if [ -e "\${FIELD}/\${selavyDir}" ]; then
                            casdaOtherDimImageSpectra+=("\${FIELD}/\${selavySpectraDir}/spec*.fits")
                            casdaOtherDimImageNoise+=("\${FIELD}/\${selavySpectraDir}/noiseSpec*.fits")
                            casdaOtherDimImageMoment0s+=("\${FIELD}/\${selavyMomentsDir}/moment0*.fits")
                            casdaOtherDimImageMoment1s+=("\${FIELD}/\${selavyMomentsDir}/moment1*.fits")
                            casdaOtherDimImageMoment2s+=("\${FIELD}/\${selavyMomentsDir}/moment2*.fits")
                            casdaOtherDimImageCubelets+=("\${FIELD}/\${selavyCubeletsDir}/cubelet*.fits")
                            casdaOtherDimImageFDF+=("")
                            casdaOtherDimImageRMSF+=("")
                            casdaOtherDimImagePol+=("")
                            if [ -e "\${FIELD}/\${selavyDir}/\${noiseMap}" ]; then
                                casdaOtherDimImageNames+=("\${FIELD}/\${selavyDir}/\${noiseMap}")
                                casdaOtherDimImageTypes+=(\${noiseType})
                            fi
                        else
                            casdaOtherDimImageSpectra+=("")
                            casdaOtherDimImageNoise+=("")
                            casdaOtherDimImageMoment0s+=("")
                            casdaOtherDimImageMoment1s+=("")
                            casdaOtherDimImageMoment2s+=("")
                            casdaOtherDimImageCubelets+=("")
                            casdaOtherDimImageFDF+=("")
                            casdaOtherDimImageRMSF+=("")
                            casdaOtherDimImagePol+=("")
                        fi
                        if [ "\${BEAM}" == "all" ] && [ "\${imageCode}" == "restored" ]; then
                            casdaOtherDimImageNames+=("\${FIELD}/\${weightsImage}")
                            casdaOtherDimImageTypes+=("\${weightsType}")
                            casdaOtherDimImageSpectra+=("")
                            casdaOtherDimImageNoise+=("")
                            casdaOtherDimImageMoment0s+=("")
                            casdaOtherDimImageMoment1s+=("")
                            casdaOtherDimImageMoment2s+=("")
                            casdaOtherDimImageCubelets+=("")
                            casdaOtherDimImageFDF+=("")
                            casdaOtherDimImageRMSF+=("")
                            casdaOtherDimImagePol+=("")
                        fi
                    fi
                done


                for subband in ${SUBBAND_WRITER_LIST_CONTCUBE}; do
        
                    for POLN in \${POL_LIST}; do
                        TTERM=0
                        setImageProperties cont
                        contImage=\${imageName}
                        pol=\$(echo "\$POLN" | tr '[:upper:]' '[:lower:]')
                        setImageProperties contcube "\$pol"
                        if [ -e "\${FIELD}/\${imageName}" ]; then
                            casdaOtherDimImageNames+=("\${FIELD}/\${imageName}")
                            casdaOtherDimImageTypes+=("\${imageType}")
                            ### Not yet writing extracted files direct to FITS, so change the assignment of fitsSuffix
                            if [ -e "\${FIELD}/\${selavyDir}" ] && [ "\${DO_RM_SYNTHESIS}" == "true" ]; then
                                prefix="\${FIELD}/\${selavyPolDir}/${SELAVY_POL_OUTPUT_BASE}"
                                casdaOtherDimImageSpectra+=("\${prefix}_spec_\${POLN}*.fits")
                                casdaOtherDimImageNoise+=("\${prefix}_noise_\${POLN}*.fits")
                                casdaOtherDimImageMoment0s+=("")
                                casdaOtherDimImageMoment1s+=("")
                                casdaOtherDimImageMoment2s+=("")
                                casdaOtherDimImageCubelets+=("")
                                casdaOtherDimImagePol+=(\${pol})
                                if [ "\${POLN}" == "Q" ]; then
                                    casdaOtherDimImageFDF+=("\${prefix}_FDF*.fits")
                                    casdaOtherDimImageRMSF+=("\${prefix}_RMSF*.fits")
                                else
                                    casdaOtherDimImageFDF+=("")
                                    casdaOtherDimImageRMSF+=("")
                                fi
                            else
                                casdaOtherDimImageSpectra+=("")
                                casdaOtherDimImageNoise+=("")
                                casdaOtherDimImageMoment0s+=("")
                                casdaOtherDimImageMoment1s+=("")
                                casdaOtherDimImageMoment2s+=("")
                                casdaOtherDimImageCubelets+=("")
                                casdaOtherDimImageFDF+=("")
                                casdaOtherDimImageRMSF+=("")
                                casdaOtherDimImagePol+=("")
                            fi
                            if [ "\${BEAM}" == "all" ] && [ "\${imageCode}" == "restored" ]; then
                                casdaOtherDimImageNames+=("\${FIELD}/\${weightsImage}")
                                casdaOtherDimImageTypes+=("\${weightsType}")
                                casdaOtherDimImageSpectra+=("")
                                casdaOtherDimImageNoise+=("")
                                casdaOtherDimImageMoment0s+=("")
                                casdaOtherDimImageMoment1s+=("")
                                casdaOtherDimImageMoment2s+=("")
                                casdaOtherDimImageCubelets+=("")
                                casdaOtherDimImageFDF+=("")
                                casdaOtherDimImageRMSF+=("")
                                casdaOtherDimImagePol+=("")
                            fi
                        fi
                    done

                done
        
            done
        
        done
        IFS="${IFS_FIELDS}"
    done

done

##############################
# Next, search for catalogues

catNames=()
catTypes=()

BEAM=all
imageCode=restored
for FIELD in \${LOCAL_FIELD_LIST}; do

    if [ "\${FIELD}" == "." ]; then
        theBeamList="all"
        TILE_LIST="${TILE_LIST}
ALL"
    else
        theBeamList=\${LOCAL_BEAM_LIST}
        getTile
        TILE_LIST="\${TILE}"
    fi

    for TILE in \${TILE_LIST}; do

        IFS="${IFS_DEFAULT}"
        for BEAM in \${theBeamList}; do               

            setImageProperties cont
            if [ -e "\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.components.xml" ]; then
                catNames+=("\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.components.xml")
                catTypes+=(continuum-component)
            fi
            if [ -e "\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.islands.xml" ]; then
                catNames+=("\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.islands.xml")
                catTypes+=(continuum-island)
            fi
            if [ "\${CONTSUB_METHOD}" == "Components" ]; then
                setContsubFilenames
                if [ -e "\${FIELD}/\${contsubDir}/\${contsubComponents}" ]; then
                    catNames+=("\${FIELD}/\${contsubDir}/\${contsubComponents}")
                    catTypes+=(continuum-component)
                fi
            fi
            if [ -e "\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.polarisation.xml" ]; then
                catNames+=("\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.polarisation.xml")
                catTypes+=(polarisation-component)
            fi
            setImageProperties spectral
            if [ -e "\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.hiobjects.xml" ]; then
                catNames+=("\${FIELD}/\${selavyDir}/selavy-\${imageName%%.fits}.hiobjects.xml")
                catTypes+=(spectral-line-emission)
            fi
        done
        IFS="${IFS_FIELDS}"
    done
done
IFS="${IFS_FIELDS}"
##############################
# Next, search for MSs

# for now, get all averaged continuum beam-wise MSs, and, if requested, all calibrated spectral MSs.
msNames=()
possibleMSnames="${msNameList}"

IFS="${IFS_FIELDS}"
for ms in \${possibleMSnames}; do

    if [ -e "\${ms}" ]; then
        msNames+=("\${ms}")
    fi

done
IFS="${IFS_DEFAULT}"

##############################
# Next, search for Calibration tables

calTables=()

# Bandpass table first
RAW_TABLE=${TABLE_BANDPASS}
SMOOTH_TABLE=${TABLE_BANDPASS}.smooth
DO_SMOOTH=${DO_BANDPASS_SMOOTH}
if [ \${DO_SMOOTH} ] && [ -e \${SMOOTH_TABLE} ]; then
    TABLE=\${SMOOTH_TABLE}
else
    TABLE=\${RAW_TABLE}
fi

if [ -e "\${TABLE}" ]; then

    # If the bandpass table is elsewhere, copy it to the BPCAL directory
    if [ ! -e "${ORIGINAL_OUTPUT}/BPCAL/\${TABLE##*/}" ]; then
        cp -r "\${TABLE}" "${ORIGINAL_OUTPUT}/BPCAL"
    fi

    calTables+=("BPCAL/\${TABLE##*/}")
fi


for BEAM in \${beams}; do
    IFS="${IFS_FIELDS}"
    for FIELD in \${FIELD_LIST}; do 
        findScienceMSnames
        if [ -e "\${FIELD}/\${gainscaltab}" ]; then
            calTables+=("\${FIELD}/\${gainscaltab}")
        fi
    done
    IFS="${IFS_DEFAULT}"
done


##############################
# Next, search for evaluation-related documents

evalNames=()

if [ "\${PREPARE_FOR_CASDA}" == "true" ]; then
    # Tar up the directory structure with the cal tables, logs,
    # slurm files & diagnostics etc, and add to the evaluation file list

    tarfile=calibration-metadata-processing-logs.tar

    # cal tables
    for((i=0;i<\${#calTables[@]};i++)); do
        table=\${calTables[i]}
        tar rvf \$tarfile "\$table"
    done

    # diagnostics directory
    tar rvf \$tarfile "\${diagnostics##*/}"
    # metadata directory
    tar rvf \$tarfile "\${metadata##*/}"
    # stats directory, tarred & compressed
    tar zcvf stats.tgz "\${stats##*/}"
    tar rvf \$tarfile stats.tgz
    # stats summary files
    for file in "${OUTPUT}"/stats-all*.txt; do
        tar rvf \$tarfile "\${file##*/}"
    done
    # slurm jobscripts directory, tarred & compressed
    tar zcvf slurmFiles.tgz "\${slurms##*/}"
    tar rvf \$tarfile slurmFiles.tgz
    # slurm output directory, tarred & compressed
    tar zcvf slurmOutputs.tgz "\${slurmOut##*/}"
    tar rvf \$tarfile slurmOutputs.tgz
    # logs directory, tarred & compressed
    tar zcvf logs.tgz "\${logs##*/}"
    tar rvf \$tarfile logs.tgz
    # parsets directory, tarred & compressed
    tar zcvf parsets.tgz "\${parsets##*/}"
    tar rvf \$tarfile parsets.tgz
    # beam logs for spectral cubes
    mkdir -p SpectralCube_BeamLogs
    cp ./*/beamlog* SpectralCube_BeamLogs
    tar rvf \$tarfile SpectralCube_BeamLogs

    evalNames+=("\$tarfile")
    evalFormats+=(calibration)
fi

if [ "\${DO_CONTINUUM_VALIDATION}" == "true" ]; then
    # Tar up the validation directory and add the xml file
    
    # Only include TILE validation, but this may not exist (ie. if
    #   there is only a single FIELD), so need to test
    BEAM="all"
    imageCode=restored

    validationDirs=()
    validationFiles=()
    if [ \${NUM_FIELDS} -gt 1 ]; then
        IFS="${IFS_FIELDS}"
        for FIELD in \${FIELD_LIST}; do
            setImageProperties cont
            if [ -e "\${FIELD}/\${validationDir}/\${validationFile}" ]; then
                validationDirs+=("\${FIELD}/\${validationDir}")
                validationFiles+=("\${FIELD}/\${validationDir}/\${validationFile}")
            fi
        done
        IFS="${IFS_DEFAULT}"
    else
        FIELD="."
        TILE="ALL"
        setImageProperties cont
        if [ -e "\${FIELD}/\${validationDir}/\${validationFile}" ]; then
            validationDirs+=("./\${validationDir}")
            validationFiles+=("./\${validationDir}/\${validationFile}")
        fi
    fi
    
    for((i=0;i<\${#validationDirs[@]};i++)); do
        dir=\${validationDirs[i]}
        echo "Have validation directory \$dir"
        cd "\${dir##/*}/.."
        echo "Moved to \$(pwd)"
        echo "Running: tar cvf \${dir##*/}.tar \\${dir##*/}"
        tar cvf "\${dir##*/}.tar" "\${dir##*/}"
        echo "Done"
        cd -
        echo "Now in \$(pwd)"
        evalNames+=("\${dir}.tar")
        evalFormats+=(tar)
    done

    for((i=0;i<\${#validationFiles[@]};i++)); do
        valfile=\${validationFiles[i]}
        echo "Have validation file \$valfile"
        evalNames+=("\${valfile}")
        evalFormats+=(validation-metrics)
    done

    # Add the archived config file, getting the relative path correct.
    evalNames+=("\${slurmOut##*/}/\${archivedConfig##*/}")
    evalFormats+=(txt)

fi

EOF
