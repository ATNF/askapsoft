#!/bin/bash -l
#
# This file holds various utility functions and environment variables
# that allow the scripts to do various things in a uniform manner.
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

# Call the createDirectories script, so that we always define the
# directories in which to put things - most importantly the stats directory.
. "${PIPELINEDIR}/createDirectories.sh"

##############################
# PIPELINE VERSION REPORTING

function reportVersion()
{

    echo "Running ASKAPsoft pipeline processing, version ${PIPELINE_VERSION}"

}

##############################
# MODULE HANDLING

# Array used to track which modules have been loaded within the pipelines
moduleTracking=()

# loadModule loads a particular module, but only if a version of it is
# not loaded already. It determines this by checking "module list".
# If it is, the module name is added to the moduleTracking array, so
# that unloadModule() can remove it.
# Takes one argument, the module name. e.g.: loadModule askapcli
function loadModule()
{
    mod=$1
    version="$(module list -t 2>&1 | grep $mod)"
    if [ "$version" == "" ]; then
        module load ${mod}
        moduleTracking+=($mod)
    fi
}

# unloadModule unloads a particular module, only if it was previously
# loaded by loadModule(). Once unloaded, the module name is removed
# from the moduleTracking array.
# Takes one argument, the module name. e.g.:  unloadModule askapcli
function unloadModule()
{
    mod=$1
    hasLoaded=false
    for m in ${moduleTracking[@]}; do
        if [ "$m" == "$mod" ]; then
            hasLoaded=true
        fi
    done
    if [ "${hasLoaded}" == "true" ]; then
        module unload ${mod}
        newTracking=()
        for m in ${moduleTracking[@]}; do
            if [ "$m" != "$mod" ]; then
                newTracking+=($m)
            fi
        done
        moduleTracking=($newTracking)
    fi
}

##############################
# ARCHIVING A CONFIG FILE

# Takes one argument, the config file
function archiveConfig()
{

    filename=$(basename "$1")
    extension="${filename##*.}"
    filename="${filename%%.*}"
    archivedConfig="$slurmOut/${filename}__${NOW}.${extension}"
    cp "$1" "$archivedConfig"

    cat >> "$archivedConfig" <<EOF

# Processed with ASKAP pipelines on ${NOW_FMT}
# Processed with ASKAPsoft version ${ASKAPSOFT_RELEASE}
# Processed with ASKAP pipeline version ${PIPELINE_VERSION}
# Processed with ACES software revision ${ACES_VERSION_USED}
EOF
    if [ "${CASA_VERSION_USED}" != "" ]; then
        echo "# Processed with CASA version ${CASA_VERSION_USED}" >> "$archivedConfig"
    fi
    if [ "${AOFLAGGER_VERSION_USED}" != "" ]; then
        echo "# Processed with AOFlagger: ${AOFLAGGER_VERSION_USED}" >> "$archivedConfig"
    fi
    if [ "${BPTOOL_VERSION_USED}" != "" ]; then
        echo "# Processed with BPTOOL version ${BPTOOL_VERSION_USED}" >> "$archivedConfig"
    fi

}

##############################
# Rejuvenation

# Takes one argument, a file or directory
function rejuvenate()
{
    if [ "$1" != "" ] && [ -e "$1" ]; then
        find "$1" -exec touch {} \;
    fi
}


##############################
# JOB NAME MANAGEMENT
#
# This sets the $sbatchfile and $jobname variables - the $sbatchfile
# should be used as the name for the slurm job file, while $jobname
# can be used both as the --job-name option to sbatch, and as the
# Description for the extractStats call. It takes two arguments, the
# first is the longer description for the basis of the slurm filename,
# and the second is the shorter one used in the slurm job name and the
# extractStats results
#  Usage:  setJob <description> <description2>
#  Requires:  $slurms, $FIELDBEAM
#  Sets:  sbatchfile=$slurms/description_FIELDBEAM.sbatch
#         jobname=description2_$fieldbeamjob
#   where fieldbeamjob removes any '_' characters from FIELDBEAM
function setJob()
{
    sbatchfile="$slurms/$1_${FIELDBEAM}.sbatch"
    fieldbeamjob=$(echo "$FIELDBEAM" | sed -e 's/_//g')
    jobname="$2_${fieldbeamjob}"
}


##############################
# JOB ID MANAGEMENT

# This string records the full list of submitted jobs as a
# comma-separated list
ALL_JOB_IDS=""
# A simple function to add a job number to the list
function addJobID()
{
    if [ "${ALL_JOB_IDS}" == "" ]; then
	ALL_JOB_IDS="$1"
    else
	ALL_JOB_IDS="${ALL_JOB_IDS},$1"
    fi
}

function reportJob()
{
    # Usage: reportJob ID "This is a long description of this job"
    echo "$1 -- $2" | tee -a "${JOBLIST}"
}

function recordJob()
{
    # Usage: recordJob ID "This is a long description of this job"
    addJobID "$1"
    reportJob "$1" "$2"
}

# Function to add a job id to a list of dependencies. Calling syntax
# is:
#  DEP=$(addDep "$DEP" "$ID")  or
#  DEP=$(addDep "$DEP" "$OLDDEP") where OLDDEP is already a dependency string.
# For instance, addDep "-d afterok:11:12:13" "-d afterok:1:2:3" will return "-d afterok:11:12:13:1:2:3"
function addDep()
{
    DEP=$1
    if [ "$2" != "" ]; then
        ID=$(echo $2 | sed -e 's/-d afterok://g')
        if [ "$1" == "" ]; then
            DEP="-d afterok"
        fi
        DEP="$DEP:$ID"
    fi
    echo "$DEP"
}

##############################
# FILENAME PARSING

# Function to return the tile name for a given field. If the field has
# been generated by tileSky, it will be of the form *_Tx-y?, where the
# * can be anything and the ? is a single letter A,B,C,...
# If the field name is of this form (splits nicely about _T), then we
# remove the final letter to get the tile.
# If it isn't, then we return the field name
# Requires: FIELD
# Sets:     TILE
function getTile()
{
    if [ "$(echo "$FIELD" | awk -F"_T" '{print NF}')" -eq 2 ]; then
        TILE=$(echo "$FIELD" | awk '{len=length($1); print substr($1,0,len-1)}')
    else
        TILE=$FIELD
    fi
}

# Function to set the directory names used by Selavy - where the
# selavy jobs are run and the sub-directories where extracted spectra
# etc go.
# Requires:
#    first argument defines the type ("cont" or "spectral")
# Uses: $imageName
function setSelavyDirs()
{
    type=$1
    # remove .fits extension if present
    im=${imageName%%.fits}
    if [ "${type}" == "cont" ]; then
        selavyDir=selavy-cont-${im}
        selavyPolDir="${selavyDir}/PolData"
    elif [ "${type}" == "spectral" ]; then
        selavyDir=selavy-spectral-${im}
        selavySpectraDir="${selavyDir}/Spectra"
        selavyMomentsDir="${selavyDir}/Moments"
        selavyCubeletsDir="${selavyDir}/Cubelets"
    fi


}

# Function to define the filenames of the model images and the
# component parset that are used by the various continuum subtraction
# tasks.
# When using the contsubCleanModelImage and contsubCmodelImage
# variables in parsets for cmodel or ccontsubtract, any .taylor.0
# suffix needs to be removed if nterms>1. This is left up to the
# calling script.
# Requires: see imageCode description
# Returns:
#  - contsubDir
#  - contsubCleanModelImage
#  - contsubCmodelImage
#  - contsubComponents
function setContsubFilenames()
{
    # Backup of the imageCode, as we change this
    imageCodeBackup=$imageCode

    # Define the base for the names, from the model image (so we don't get the .restored)
    imageCode=image
    setImageProperties cont
    imageStub=${imageName%%.fits}

    
    ####
    # First the contsub directory
    contsubDir=ContSubBeam${FIELDBEAM}
    ####
    contsubCleanModelImage=$imageStub
    contsubCleanModelType=${imageType}
    contsubCleanModelLabel="Continuum model image from clean model"
    ####
    # Next the model image created by cmodel
    contsubCmodelImage=model.contsub.${imageStub}
    contsubCmodelType="${imageType}"
    contsubCmodelLabel="Continuum model image from catalogue"
    ####
    # Finally the components parset
    contsubComponents="modelComponents.contsub.${imageStub}.in"

    # Restore the imageCode to what it was
    imageCode=$imageCodeBackup
    unset imageCodeBackup
}

# Function to define a set of variables describing an image - its
# name, image type (for CASDA), and label (for preview images), based
# on a type and BEAM/POL/FIELD information
# Requires:
#  * FIELD (the special value "." means a mosaic of multiple fields)
#  * TILE (only for FIELD="." - special value of "ALL" means the full
#          mosaic over all fields/tiles)
#  * BEAM
#  * imageCode (one of restored|altrestored|contsub|image|residual|psf|psfimage)
#  * pol (lower case polarisation i/q/u/v etc)
#  * TTERM (Taylor term: 0,1,2,...) Can be blank ("" ie. unset), which
#     defaults to zero
#  * NUM_TAYLOR_TERMS (number of taylor terms being solved for. 1=no MFS)
#  * IMAGE_BASE_CONT,IMAGE_BASE_CONTCUBE,IMAGE_BASE_SPECTRAL
#  * subband (only used if NUM_SPECTRAL_CUBES or NUM_SPECTRAL_CUBES_CONTCUBE > 1)
#  * DO_ALT_IMAGER_{CONT,CONTCUBE,SPECTRAL} (if required)
#  * IMAGETYPE_{CONT,CONTCUBE,SPECTRAL}, to determine whether a .fits
#    extension is added to the filenames.
#  * ALT_IMAGER_SINGLE_FILE or ALT_IMAGER_SINGLE_FILE_CONTCUBE (for
#         spectral-line or continuum cube respectively, for ALT_IMAGER=true)
#  * PROJECT_ID - for the name of the continuum validation file
# Available upon return:
#  * imageBase
#  * imageName (the filename for the image)
#  * imageType (the image type used by CASDA - eg. cont_restored_T0)
#  * label (the title for the preview image - only used for continuum)
#  * weightsImage, weightsType, weightsLabel (as above)
# Usage: setImageProperties <type>
#    type = cont | spectral | contcube
function setImageProperties()
{
    type=$1

    imSuffix=""
    setImageBase "$type"

    needToUnsetTTerm=false
    if [ "$TTERM" == "" ]; then
        TTERM=0
        needToUnsetTTerm=true
    fi

    band=""
    doAlt=false
    extension=""

    if [ "$type" == "cont" ]; then
        typebase="cont"
        labelbase="continuum image"
        if [ "${NUM_TAYLOR_TERMS}" == "" ] || [ "${NUM_TAYLOR_TERMS}" -eq 1 ]; then
            imSuffix=""
            typeSuffix="T0"
        else
            imSuffix=".taylor.$TTERM"
            typeSuffix="T${TTERM}"
        fi
        if [ "${IMAGETYPE_CONT}" == "fits" ]; then
            extension=".fits"
        fi
        # Add the writer information for the askap_imager case, but not when we have a single-file FITS output
        if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
            doAlt=true
        fi
    elif [ "$type" == "spectral" ]; then
        typebase="spectral"
        labelbase="spectral cube"
        typeSuffix="3d"
        if [ "${IMAGETYPE_SPECTRAL}" == "fits" ]; then
            extension=".fits"
        fi
        # Add the writer information for the askap_imager case, but not when we have a single-file FITS output
        if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then
            doAlt=true
            if [ "${ALT_IMAGER_SINGLE_FILE}" != "true" ]; then
                band="wr.${subband}."
            fi
        fi
    elif [ "$type" == "contcube" ]; then
        typebase="cont"
        labelbase="continuum cube"
        typeSuffix="3d"
        if [ "${IMAGETYPE_CONTCUBE}" == "fits" ]; then
            extension=".fits"
        fi
        # Add the writer information for the askap_imager case, but not when we have a single-file FITS output
        if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ]; then
            doAlt=true
            if [ "${ALT_IMAGER_SINGLE_FILE_CONTCUBE}" != "true" ]; then
                band="wr.${subband}."
            fi
        fi
    else
        echo "ERROR - bad type for setImageProperies: \"$type\""
    fi

    if [ "${FIELD}" == "." ]; then
        beamSuffix="mosaic"
    else
        if [ "${BEAM}" == "all" ]; then
            beamSuffix="mosaic"
        else
            beamSuffix="beam ${BEAM}"
        fi
    fi

    base="${band}${imageBase}${imSuffix}"

    weightsImage="weights.${base}${extension}"
    weightsType="${typebase}_weight_$typeSuffix"
    weightsLabel="Weights image, $beamSuffix"

    # Set the imageName according to the image code.
    # For the restored images, we need to have image.restored.$base
    # when we are using the new imager, except for the continuum
    # image, since we use solverpercore=false. When using cimager, we
    # revert to image.$base.restored.
    # Same idea for contsub and altrestored.
    if [ "$imageCode" == "restored" ]; then
        if [ "${doAlt}" == "true" ] && [ "${type}" != "cont" ]; then
            imageName="image.restored.${base}"
        else
            imageName="image.${base}.restored"
        fi
        imageType="${typebase}_restored_$typeSuffix"
        label="Restored ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "contsub" ]; then
        if [ "${doAlt}" == "true" ] && [ "${type}" != "cont" ]; then
            imageName="image.restored.${base%%.fits}.contsub"
        else
            imageName="image.${base%%.fits}.restored.contsub"
        fi
        imageType="${typebase}_restored_$typeSuffix"
        label="Restored, Continuum-subtracted ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "altrestored" ]; then
        if [ "${doAlt}" == "true" ] && [ "${type}" != "cont" ]; then
            imageName="image.restored.${base}.alt"
        else
            imageName="image.${base}.alt.restored"
        fi
        imageType="${typebase}_restored_$typeSuffix"
        label="Restored ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "image" ]; then
        imageName="image.${base}"
        imageType="${typebase}_cleanmodel_$typeSuffix"
        label="Clean model ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "residual" ]; then
        imageName="residual.${base}"
        imageType="${typebase}_residual_$typeSuffix"
        label="Clean residual ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "sensitivity" ]; then
        imageName="sensitivity.${base}"
        imageType="${typebase}_sensitivity_$typeSuffix"
        label="Sensitivity ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "psf" ]; then
        imageName="psf.${base}"
        imageType="${typebase}_psfnat_$typeSuffix"
        label="PSF ${labelbase}, $beamSuffix"
    elif [ "$imageCode" == "psfimage" ]; then
        imageName="psf.image.${base}"
        imageType="${typebase}_psfprecon_$typeSuffix"
        label="Preconditioned PSF ${labelbase}, $beamSuffix"
    else
        echo "WARNING - unknown image code \"${imageCode}\""
    fi

    imageName="${imageName}${extension}"

    # Definitions use by Selavy jobs
    setSelavyDirs $type
    if [ "${type}" == "cont" ]; then
        noiseMap=noiseMap.${imageName%%.fits}${extension}
        noiseType="cont_noise_T0"
        noiseLabel="Continuum image noise map"
        thresholdMap=detThresh.${imageName%%.fits}${extension}
        meanMap=meanMap.${imageName%%.fits}${extension}
        snrMap=snrMap.${imageName%%.fits}${extension}
        compMap=componentMap_${imageName%%.fits}${extension}
        compMapType="cont_components_T0"
        compMapLabel="Continuum component map"
        compResidual=componentResidual_${imageName%%.fits}${extension}
        compResidualType="cont_fitresidual_T0"
        compResidualLabel="Continuum component residual map"
        validationDir=${imageName%%.fits}_continuum_validation_selavy_snr5.0_int
        validationFile=${PROJECT_ID}_CASDA_continuum_validation.xml
    elif [ "${type}" == "spectral" ]; then
        noiseMap=noiseMap.${imageName%%.fits}${extension}
        noiseType="spectral_noise_3d"
        noiseLabel="Spectral cube noise map"
        thresholdMap=detThresh.${imageName%%.fits}${extension}
        meanMap=meanMap.${imageName%%.fits}${extension}
        snrMap=snrMap.${imageName%%.fits}${extension}
    fi

    if [ "$needToUnsetTTerm" == "true" ]; then
        unset TTERM
    fi

}


# Function to set the base name for an image/image cube. Can handle
# different image types - continuum, continuum cube & spectral cube.
# The name is formed based on the type, the FIELD, the BEAM (and the
# polarisation in the case of the continuum cube).
# Requires:
#  * FIELD (the special value "." means a mosaic of multiple fields)
#  * TILE (only for FIELD="." - special value of "ALL" means the full
#          mosaic over all fields/tiles)
#  * BEAM
#  * pol (lower case polarisation i/q/u/v etc)
#  * IMAGE_BASE_CONT,IMAGE_BASE_CONTCUBE,IMAGE_BASE_SPECTRAL
# Available upon return:
#  * imageBase
# Usage: setImageBase <type>
#    type = cont | spectral | contcube
function setImageBase()
{

    type="$1"

    if [ "$type" == "cont" ]; then
        imageBase=${IMAGE_BASE_CONT}
    elif [ "$type" == "contcube" ]; then
        imageBase=${IMAGE_BASE_CONTCUBE}
        sedstr="s/^i\./$pol\./g"
        imageBase=$(echo "${imageBase}" | sed -e "$sedstr")
        sedstr="s/%p/$pol/g"
        imageBase=$(echo "${imageBase}" | sed -e "$sedstr")
    elif [ "$type" == "spectral" ]; then
        imageBase=${IMAGE_BASE_SPECTRAL}
    else
        echo "ERROR - bad type for setImageBase: \"$type\""
    fi

    if [ "${FIELD}" == "." ]; then
        if [ "${TILE}" == "ALL" ]; then
            imageBase="${imageBase}"
        else
            imageBase="${imageBase}.${TILE}"
        fi
    else
        imageBase="${imageBase}.${FIELD}"
        if [ "${BEAM}" == "all" ]; then
            imageBase="${imageBase}.linmos"
        else
            imageBase="${imageBase}.beam${BEAM}"
        fi
    fi

    imageBase=$(echo $imageBase | sed -e 's/ /_/g')

}

function getMSname()
{
    # Returns just the filename of the science MS, stripping off the
    # leading directories and the .ms suffix. For example, the MS
    # /path/to/2016-01-02-0345.ms returns 2016-01-02-0345
    # Usage:     getMSname MS
    # Returns:   $msname

    msname=${1##*/}
    msname=${msname%%.*}
}


# A function to work out the measurement set names for the
# full-resolution, spectral-line and channel-averaged cases, given the
# current BEAM.
# Does wildcard replacement of %b (->FIELD.beamBEAM) and %s (->SB_SCIENCE)
# Requires the following:
#   * MS_BASE_SCIENCE
#   * BEAM
#   * SB_SCIENCE
#   * DO_COPY_SL
#   * MS_SCIENCE_AVERAGE  (can be blank)
#   * nbeams
#   * KEEP_RAW_AV_MS
#   * GAINS_CAL_TABLE
#   * DO_SELFCAL
#   * DO_APPLY_CAL_SL
#   *
function findScienceMSnames()
{

    # 1. Get the value for $msSci (the un-averaged MS)
    if [ "$(echo "${MS_BASE_SCIENCE}" | grep %b)" != "" ]; then
        # If we are here, then $MS_BASE_SCIENCE has a %b that needs to be
        # replaced by the current ${FIELD}.beam$BEAM value.
        # Also include the $FIELD value to uniquely identify it
        sedstr="s|%b|${FIELD}\.beam${BEAM}|g"
        msSci=$(echo "${MS_BASE_SCIENCE}" | sed -e "$sedstr")
    else
        # If we are here, then there is no %b, and we just append
        # _${FIELD}.${BEAM} to the MS name
        sedstr="s/\.ms/_${FIELD}\.beam${BEAM}\.ms/g"
        msSci=$(echo "${MS_BASE_SCIENCE}" | sed -e "$sedstr")
    fi
    # If time-splitting is sought, append the TimeWindow Number to the 
    # measurement set:
    if [ "${DO_SPLIT_TIMEWISE}" == "true" ]; then
        sedstr="s/\.ms/\.timeWin${TimeWindow}\.ms/g"
        msSci=$(echo "${msSci}" | sed -e "$sedstr")
    fi

    # Replace the %s wildcard with the SBID
    sedstr="s|%s|${SB_SCIENCE}|g"
    msSci=$(echo "${msSci}" | sed -e "$sedstr")

    # Replace any spaces (e.g. from the FIELD name) with an underscore
    msSci=$(echo $msSci | sed -e 's/ /_/g')

    if [ "${DO_COPY_SL}" == "true" ]; then
        # If we make a copy of the spectral-line MS, then append '_SL'
        # to the MS name before the suffix for the MS used for
        # spectral-line imaging
        sedstr="s/\.ms/_SL\.ms/g"
        msSciSL=$(echo "${msSci}" | sed -e "$sedstr")
    else
        # If we aren't copying, just use the original full-resolution dataset
        msSciSL=${msSci}
    fi

    # 2. Get the value for $msSciAv (after averaging)
    if [ "${MS_SCIENCE_AVERAGE}" == "" ]; then
        # If we are here, then the user has not provided a value for
        # MS_SCIENCE_AVERAGE, and we need to work out $msSciAv from
        # $msSci
        sedstr="s/\.ms/_averaged\.ms/g"
        msSciAv=$(echo "$msSci" | sed -e "$sedstr")
    else
        # If we are here, then the user has given a specific filename
        # for MS_SCIENCE_AVERAGE. In this case, we can either replace
        # the %b with the beam number, or leave as is (but give a
        # warning).
        if [ "$(echo "${MS_SCIENCE_AVERAGE}" | grep %b)" != "" ]; then
            # If we are here, then $MS_SCIENCE_AVERAGE has a %b that
            # needs to be replaced by the current $BEAM value
            sedstr="s|%b|${FIELD}\.beam${BEAM}|g"
            msSciAv=$(echo "${MS_SCIENCE_AVERAGE}" | sed -e "$sedstr")
        else
            msSciAv=${MS_SCIENCE_AVERAGE}
            if [ "$nbeams" -gt 1 ]; then
                # Only give the warning if there is more than one beam
                # (which means we're using the same MS for them)
                echo "Warning! Using ${msSciAv} as averaged MS for beam ${BEAM}"
            fi
        fi
        # Replace the %s wildcard with the SBID
        sedstr="s|%s|${SB_SCIENCE}|g"
        msSciAv=$(echo "${msSciAv}" | sed -e "$sedstr")

    fi
    # Replace any spaces (e.g. from the FIELD name) with an underscore
    msSciAv=$(echo $msSciAv | sed -e 's/ /_/g')

    # We now define the name of the calibrated averaged dataset
    if [ "${KEEP_RAW_AV_MS}" == "true" ]; then
        # If we are keeping the raw data, need a new MS name
        sedstr="s/\.ms$/_cal\.ms/g"
        msSciAvCal=$(echo "$msSciAv" | sed -e "$sedstr")
    else
        # Otherwise, apply the calibration to the raw data
        msSciAvCal=$msSciAv
    fi

    if [ "${GAINS_CAL_TABLE}" == "" ]; then
        # The name of the gains cal table is blank, so turn off
        # selfcal & cal-apply for the SL case
        if [ "${DO_SELFCAL}" == "true" ]; then
            DO_SELFCAL=false
            echo "Gains cal filename (GAINS_CAL_TABLE) blank, so turning off selfcal"
        fi
        if [ "${DO_APPLY_CAL_SL}" == "true" ]; then
            DO_APPLY_CAL_SL=false
            echo "Gains cal filename (GAINS_CAL_TABlE) blank, so turning off SL cal apply"
        fi
    else
        # Otherwise, need to replace any %b with the current BEAM, if there is one present
        if [ "$(echo "${GAINS_CAL_TABLE}" | grep %b)" != "" ]; then
            # We have a %b that needs replacing
            sedstr="s|%b|${FIELD}\.beam${BEAM}|g"
            gainscaltab="$(echo "${GAINS_CAL_TABLE}" | sed -e "$sedstr")"
        else
            # just use filename as provided
            gainscaltab="${GAINS_CAL_TABLE}"
        fi
        # Replace the %s wildcard with the SBID
        sedstr="s|%s|${SB_SCIENCE}|g"
        gainscaltab=$(echo "${gainscaltab}" | sed -e "$sedstr")

    fi
    # Replace any spaces (e.g. from the FIELD name) with an underscore
    gainscaltab=$(echo $gainscaltab | sed -e 's/ /_/g')

}

function find1934MSnames()
{
    if [ "$(echo "${MS_BASE_1934}" | grep %b)" != "" ]; then
        # If we are here, then $MS_BASE_1934 has a %b that
        # needs to be replaced by the current $BEAM value
        sedstr="s|%b|beam${BEAM}|g"
        msCal=$(echo "${MS_BASE_1934}" | sed -e "$sedstr")
    else
        msCal=${MS_BASE_1934}
        echo "Warning! Using ${msCal} as 1934-638 MS for beam ${BEAM}"
    fi
    # Replace the %s wildcard with the SBID
    sedstr="s|%s|${SB_1934}|g"
    msCal=$(echo "${msCal}" | sed -e "$sedstr")

}

# Function to provide the name of the mslist metadata file for the
# science data. If we are merging MSs, this will relate to the final
# merged version (which will be created on a per-beam
# basis). Otherwise (and this is what was the standard, indeed only
# approach), we have a single name based on the input
# MS_INPUT_SCIENCE.
# Requires:
#  * MS_INPUT_SCIENCE - passed to getMSname()
#  * NEED_TO_MERGE_SCI
#  * BEAM (only when NEED_TO_MERGE_SCI == true)
# Returns: MS_METADATA
function findScienceMSmetadataFile()
{
    if [ "${NEED_TO_MERGE_SCI}" == "true" ]; then
        findScienceMSnames
        getMSname $msSci
        MS_METADATA="$metadata/mslist-${msname}_beam${BEAM}.txt"
    else
        getMSname "${MS_INPUT_SCIENCE}"
        MS_METADATA="$metadata/mslist-${msname}.txt"
    fi
}

# Function to provide the name of the mslist metadata file for the
# science data. If we are merging MSs, this will relate to the final
# merged version (which will be created on a per-beam
# basis). Otherwise (and this is what was the standard, indeed only
# approach), we have a single name based on the input
# MS_INPUT_SCIENCE.
# Requires:
#  * MS_INPUT_1934 - passed to getMSname()
#  * NEED_TO_MERGE_CAL
#  * BEAM (only when NEED_TO_MERGE_CAL == true)
# Returns: MS_METADATA_CAL
function find1934MSmetadataFile()
{
    if [ "${NEED_TO_MERGE_SCI}" == "true" ]; then
        find1934MSnames
        getMSname $msCal
        MS_METADATA_CAL="$metadata/mslist-cal-${msname}_beam${BEAM}.txt"
    else
        getMSname "${MS_INPUT_1934}"
        MS_METADATA_CAL="$metadata/mslist-cal-${msname}.txt"
    fi
}


# Function to return a list of polarisations, separated by
# spaces, converted from the user-input list of comma-separated
# polarisations for the continuum cubes
#  Required inputs:
#     * CONTCUBE_POLARISATIONS - something like "I,Q,U,V"
#  Returns: $POL_LIST (would convert above to "I Q U V")
function getPolList()
{
    POL_LIST=$(echo "$CONTCUBE_POLARISATIONS" | sed -e 's/,/ /g')

}

# Function to set the cleaning parameters that can potentially depend
# on the self-calibration loop number. The array index is taken from
# the value of $LOOP - if this is blank then zero (the first value) is
# used
# Requires/uses:
#  * LOOP
#  * CLEAN_THRESHOLD_MAJORCYCLE_ARRAY
#  * CLEAN_NUM_MAJORCYCLES_ARRAY
#  * CLEAN_ALGORITHM_ARRAY
#  * CLEAN_MINORCYCLE_NITER_ARRAY
#  * CLEAN_THRESHOLD_MINORCYCLE_ARRAY
#  * CLEAN_GAIN_ARRAY
#  * CLEAN_PSFWIDTH_ARRAY 
#  * CLEAN_SCALES_ARRAY
# Returns:
#  * loopParams
function cimagerSelfcalLoopParams()
{
    if [ "$LOOP" == "" ]; then
        loopval=0
    else
        loopval=$LOOP
    fi
    loopParams="# Parameters set for loop $loopval
Cimager.threshold.majorcycle                    = ${CLEAN_THRESHOLD_MAJORCYCLE_ARRAY[$loopval]}
Cimager.ncycles                                 = ${CLEAN_NUM_MAJORCYCLES_ARRAY[$loopval]}
Cimager.solver.Clean.algorithm                  = ${CLEAN_ALGORITHM_ARRAY[$loopval]}
Cimager.solver.Clean.niter                      = ${CLEAN_MINORCYCLE_NITER_ARRAY[$loopval]}
Cimager.threshold.minorcycle                    = ${CLEAN_THRESHOLD_MINORCYCLE_ARRAY[$loopval]}
Cimager.solver.Clean.gain                       = ${CLEAN_GAIN_ARRAY[$loopval]}
Cimager.solver.Clean.psfwidth                   = ${CLEAN_PSFWIDTH_ARRAY[$loopval]}
Cimager.solver.Clean.scales                     = ${CLEAN_SCALES_ARRAY[$loopval]}
"
}

# Function to return self-calibration-loop-dependant parameters
# governing data selection. This returns parset parameters suitable
# for either Cimager or Ccalibrator (the task name is given as the
# first argument to the function)
# Use: dataSelectionSelfcalLoop
# Requires/uses:
#   * LOOP
#   * CIMAGER_MINUV_ARRAY or
#   * CCALIBRATOR_MINUV_ARRAY
# Returns:
#   * dataSelectionParamsIm
#   * dataSelectionParamsCal
function dataSelectionSelfcalLoop()
{
    dataSelectionParamsIm=""
    dataSelectionParamsCal=""
    needToUnsetLoop=false
    if [ "$LOOP" == "" ]; then
        LOOP=0
        needToUnsetLoop=true
    fi
    if [ "${CIMAGER_MINUV_ARRAY[$LOOP]}" -gt 0 ]; then
        dataSelectionParamsIm="Cimager.MinUV   = ${CIMAGER_MINUV_ARRAY[$LOOP]}"
    fi
    if [ "${CIMAGER_MAXUV_ARRAY[$LOOP]}" -gt 0 ]; then
        dataSelectionParamsIm="Cimager.MaxUV   = ${CIMAGER_MAXUV_ARRAY[$LOOP]}"
    fi
    if [ "${CCALIBRATOR_MINUV_ARRAY[$LOOP]}" -gt 0 ]; then
        dataSelectionParamsCal="Ccalibrator.MinUV   = ${CCALIBRATOR_MINUV_ARRAY[$LOOP]}"
    fi
    if [ "${CCALIBRATOR_MAXUV_ARRAY[$LOOP]}" -gt 0 ]; then
        dataSelectionParamsCal="Ccalibrator.MaxUV   = ${CCALIBRATOR_MAXUV_ARRAY[$LOOP]}"
    fi
    if [ "$needToUnsetLoop" == "true" ]; then
        unset LOOP
    fi
}


##############################
# BEAM FOOTPRINTS AND CENTRES

# Function to set the arguments to footprint.py, based on the
# input parameters. They only contribute if they are not blank.
# The arguments that are set and the parameters used are:
#  * summary output (the -t flag)
#  * name (-n $FP_NAME),
#  * band (-b $FREQ_BAND_NUMBER)
#  * PA (-a $FP_PA)
#  * pitch (-p $FP_PITCH)
# Returns: $footprintArgs
function setFootprintArgs()
{

    # Specify the name of the footprint
    if [ "$FP_NAME" != "" ]; then
        footprintArgs="$footprintArgs -n $FP_NAME"
    fi

    # Specify the band number (from BETA days) to get default pitch values
    if [ "$FREQ_BAND_NUMBER" != "" ]; then
        footprintArgs="$footprintArgs -b $FREQ_BAND_NUMBER"
    fi

    # Specify the position angle of the footprint
    if [ "$FP_PA" != "" ]; then
        footprintArgs="$footprintArgs -a $FP_PA"
    fi

    # Specify the pitch of the footprint - separation of beams
    if [ "$FP_PITCH" != "" ]; then
        footprintArgs="$footprintArgs -p $FP_PITCH"
    fi
}

function setFootprintFile()
{
    # Function to define a file containing the beam locations for the
    # requested footprint, which is created for a given run and a given
    # field name. Need a new one for each run of the pipeline as we
    # may (conceivably) change footprints from run to run.
    # Format will be
    # footprintOutput-sbSBID-FIELDNAME-FOOTPRINTNAME-bandBAND-aPA-pPITCH.txt
    # where blank parameters are left out.
    #  Required available parameters:
    #     * FIELD - name of field
    #     * SB_SCIENCE - SBID
    #  Returns: $footprintOut

    footprintOut="${metadata}/footprintOutput"
    if [ "$SB_SCIENCE" != "" ]; then
        footprintOut="$footprintOut-sb${SB_SCIENCE}"
    fi
    footprintOut="$footprintOut-${FIELD}.txt"
}

function getBeamOffsets()
{
    # Function to return beam offsets (as would be used in a linmos
    # parset) for the full set of beams for a given field.
    #  Required available parameters
    #     * FIELD - name of field
    #     * SB_SCIENCE
    #     * BEAM_MAX - how many beams to consider
    #  Returns: $LINMOS_BEAM_OFFSETS (in the process, setting $footprintOut)

    setFootprintFile
    LINMOS_BEAM_OFFSETS=$(grep -A$((BEAM_MAX+1)) Beam "${footprintOut}" | tail -n $((BEAM_MAX+1)) | sed -e 's/(//g' | sed -e 's/)//g' | awk '{printf "linmos.feeds.beam%02d = [%6.3f, %6.3f]\n",$1,-$4,$5}')
}

function getBeamCentre()
{
    # Function to return the centre direction of a given beam
    #  Required available parameters:
    #     * beamFromCLI - if true, have used footprint, else have used footprint.py
    #     * SB_SCIENCE
    #     * FIELD - name of field
    #     * BEAM - the beam ID to obtain the centre for
    #     * BEAM_MAX - how many beams to consider
    #  Returns: $DIRECTION (in the process, setting $footprintOut, $ra, $dec)

    awkTest="\$1==$BEAM"
    setFootprintFile
    if [ "$beamFromCLI" == "true" ]; then
        ra=$(awk "$awkTest" "${footprintOut}" | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g'| awk '{print $4}')
        ra=$(echo "$ra" | awk -F':' '{printf "%sh%sm%s",$1,$2,$3}')
        dec=$(awk "$awkTest" "${footprintOut}" | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g'| awk '{print $5}')
        dec=$(echo "$dec" | awk -F':' '{printf "%s.%s.%s",$1,$2,$3}')
    else
        ra=$(grep -A$((BEAM_MAX+1)) Beam "${footprintOut}" | tail -n $((BEAM_MAX+1)) | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g' | awk "$awkTest" | awk '{print $6}')
        ra=$(echo "$ra" | awk -F':' '{printf "%sh%sm%s",$1,$2,$3}')
        dec=$(grep -A$((BEAM_MAX+1)) Beam "${footprintOut}" | tail -n $((BEAM_MAX+1)) | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g' | awk "$awkTest" | awk '{print $7}')
        dec=$(echo "$dec" | awk -F':' '{printf "%s.%s.%s",$1,$2,$3}')
    fi
    DIRECTION="[$ra, $dec, J2000]"
}


##############################
# JOB STATISTIC MANAGEMENT

function writeStats()
{
    # usage: writeStats ID DESC RESULT NCORES REAL USER SYS VM RSS STARTTIME format
    #   where format is either txt or csv. Anything else defaults to txt
    format=${11}
    if [ $# -ge 10 ] && [ "$format" == "csv" ]; then
	echo "$@" | awk '{printf "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10}'
    else
	echo "$@" | awk '{printf "%10s%10s%50s%9s%10s%10s%10s%10s%10s%25s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9,$10}'
    fi
}

function writeStatsHeader()
{
    # usage: writeStatsHeader [format]
    #   where format is either txt or csv. Anything else defaults to txt
    if [ $# -ge 1 ] && [ "$1" == "csv" ]; then
	format="csv"
    else
	format="txt"
    fi
    writeStats "JobID" "nCores" "Description" "Result" "Real" "User" "System" "PeakVM" "PeakRSS" "StartTime" $format
}

function extractStatsNonStandard()
{
    # usage: extractStatsNonStandard logfile nCores ID ResultCode Description [format]
    # format is optional. If not provided, output is written to stdout
    #   if provided, it is assumed to be a list of suffixes - these can be either txt or csv.
    #      If txt - output is written to $stats/stats-ID-DESCRIPTION.txt as space-separated ascii
    #      If csv - output is written to $stats/stats-ID-DESCRIPTION.csv as comma-separated values
    #

    STATS_LOGFILE=$1
    NUM_CORES=$2
    STATS_ID=$3
    RESULT=$4
    STATS_DESC=$5

    if [ "$RESULT" -eq 0 ]; then
        RESULT_TXT="OK"
    else
        RESULT_TXT="FAIL"
    fi

    START_TIME_JOB=$(grep "STARTTIME=" ${log}.timing | head -n 1 | awk -F '=' '{print $2}')

    TIME_JOB_REAL=$(grep real ${log}.timing | tail -n 1 | awk '{print $2}')
    TIME_JOB_USER=$(grep user ${log}.timing | tail -n 1 | awk '{print $2}')
    TIME_JOB_SYS=$(grep sys ${log}.timing | tail -n 1 | awk '{print $2}')

    PEAK_VM_MASTER="---"
    PEAK_RSS_MASTER="---"

    if [ $# -lt 6 ]; then
	formatlist="stdout"
    else
	formatlist=$6
    fi

    for format in $(echo "$formatlist" | sed -e 's/,/ /g'); do

	if [ "$format" == "txt" ]; then
	    output="${stats}/stats-${STATS_ID}-${STATS_DESC}.txt"
	elif [ "$format" == "csv" ]; then
	    output="${stats}/stats-${STATS_ID}-${STATS_DESC}.csv"
	else
	    output=/dev/stdout
	fi

	writeStatsHeader "$format" > "$output"
        writeStats "$STATS_ID" "$NUM_CORES" "$STATS_DESC"              "$RESULT_TXT" "$TIME_JOB_REAL" "$TIME_JOB_USER" "$TIME_JOB_SYS" "$PEAK_VM_MASTER"  "$PEAK_RSS_MASTER"  "$START_TIME_JOB" "$format" >> "$output"

    done
}


function extractStats()
{
    # usage: extractStats logfile nCores ID ResultCode Description [format]
    # format is optional. If not provided, output is written to stdout
    #   if provided, it is assumed to be a list of suffixes - these can be either txt or csv.
    #      If txt - output is written to $stats/stats-ID-DESCRIPTION.txt as space-separated ascii
    #      If csv - output is written to $stats/stats-ID-DESCRIPTION.csv as comma-separated values

    STATS_LOGFILE=$1
    NUM_CORES=$2
    STATS_ID=$3
    RESULT=$4
    STATS_DESC=$5

    if [ "$RESULT" -eq 0 ]; then
        RESULT_TXT="OK"
    else
        RESULT_TXT="FAIL"
    fi

    parseLog "${STATS_LOGFILE}"

    if [ $# -lt 6 ]; then
	formatlist="stdout"
    else
	formatlist=$6
    fi

    for format in $(echo "$formatlist" | sed -e 's/,/ /g'); do

	if [ "$format" == "txt" ]; then
	    output="${stats}/stats-${STATS_ID}-${STATS_DESC}.txt"
	elif [ "$format" == "csv" ]; then
	    output="${stats}/stats-${STATS_ID}-${STATS_DESC}.csv"
	else
	    output=/dev/stdout
	fi

	writeStatsHeader "$format" > "$output"
	if [ "$(grep -c "(1, " "${STATS_LOGFILE}")" -gt 0 ]; then
	    writeStats "$STATS_ID" "$NUM_CORES" "${STATS_DESC}_master"     "$RESULT_TXT" "$TIME_JOB_REAL" "$TIME_JOB_USER" "$TIME_JOB_SYS" "$PEAK_VM_MASTER"  "$PEAK_RSS_MASTER"  "$START_TIME_JOB" "$format" >> "$output"
	    writeStats "$STATS_ID" "$NUM_CORES" "${STATS_DESC}_workerPeak" "$RESULT_TXT" "$TIME_JOB_REAL" "$TIME_JOB_USER" "$TIME_JOB_SYS" "$PEAK_VM_WORKERS" "$PEAK_RSS_WORKERS" "$START_TIME_JOB" "$format" >> "$output"
	    writeStats "$STATS_ID" "$NUM_CORES" "${STATS_DESC}_workerAve"  "$RESULT_TXT" "$TIME_JOB_REAL" "$TIME_JOB_USER" "$TIME_JOB_SYS" "$AVE_VM_WORKERS"  "$AVE_RSS_WORKERS"  "$START_TIME_JOB" "$format" >> "$output"
	else
	    writeStats "$STATS_ID" "$NUM_CORES" "$STATS_DESC"              "$RESULT_TXT" "$TIME_JOB_REAL" "$TIME_JOB_USER" "$TIME_JOB_SYS" "$PEAK_VM_MASTER"  "$PEAK_RSS_MASTER"  "$START_TIME_JOB" "$format" >> "$output"
	fi

    done

}

function parseLog()
{

    logfile=$1

    TIME_JOB_REAL="---"
    TIME_JOB_SYS="---"
    TIME_JOB_USER="---"
    PEAK_VM_MASTER="---"
    PEAK_RSS_MASTER="---"
    START_TIME_JOB="---"

    if [ "${NUM_CORES}" -ge 2 ] && [ "$(grep -c "(1, " "$logfile")" -gt 0 ]; then
        # if here, job was a distributed job
        # Get the master node's first log message, and extract the time stamp
        START_TIME_JOB=$(grep "(0, " "$logfile" | head -1 | awk '{printf "%sT%s",$5,$6}' | sed -e 's/^\[//g' | sed -e 's/\]$//g')
        if [ "$(grep -c "Total times" "$logfile")" -gt 0 ]; then
            TIME_JOB_REAL=$(grep "Total times" "$logfile" | tail -1 | awk '{print $16}')
            TIME_JOB_SYS=$(grep "Total times" "$logfile" | tail -1 | awk '{print $14}')
            TIME_JOB_USER=$(grep "Total times" "$logfile" | tail -1 | awk '{print $12}')
        fi
        if [ "$(grep "(0, " "$logfile" | grep -c "PeakVM")" -gt 0 ]; then
            PEAK_VM_MASTER=$(grep "(0, " "$logfile" | grep "PeakVM" | tail -1 | awk '{print $12}')
            PEAK_RSS_MASTER=$(grep "(0, " "$logfile" | grep "PeakVM" | tail -1 | awk '{print $15}')
        fi
	findWorkerStats "$logfile"
    else
        # if here, it was a serial job
        # Can log with either (-1 or (0 as the rank, so instead get the first INFO line & extract time stamp
        START_TIME_JOB=$(grep "INFO" "$logfile" | head -1 | awk '{printf "%sT%s",$5,$6}' | sed -e 's/^\[//g' | sed -e 's/\]$//g')
        if [ "$(grep -c "Total times" "$logfile")" -gt 0 ]; then
            TIME_JOB_REAL=$(grep "Total times" "$logfile" | tail -1 | awk '{print $16}')
            TIME_JOB_SYS=$(grep "Total times" "$logfile" | tail -1 | awk '{print $14}')
            TIME_JOB_USER=$(grep "Total times" "$logfile" | tail -1 | awk '{print $12}')
        fi
        if [ "$(grep -c "PeakVM" "$logfile")" -gt 0 ]; then
            PEAK_VM_MASTER=$(grep "PeakVM" "$logfile" | tail -1 | awk '{print $12}')
            PEAK_RSS_MASTER=$(grep "PeakVM" "$logfile" | tail -1 | awk '{print $15}')
        fi
    fi

}

function findWorkerStats()
{
    logfile=$1
    tmpfile=${tmp}/tmpout

    PEAK_VM_WORKERS="---"
    PEAK_RSS_WORKERS="---"
    AVE_VM_WORKERS="---"
    AVE_RSS_WORKERS="---"

    grep "PeakVM" "$logfile" | grep -v "(0, " > "$tmpfile"

    if [ "$(wc -l "$tmpfile" | awk '{print $1}')" -gt 0 ]; then

        awkfile="$tmp/workerstats.awk"
        if [ ! -e "$awkfile" ]; then
            cat > "$awkfile" <<EOF
BEGIN {
    i=0;
    sumV=0.;
    sumR=0.;
}
{
    if(NF==16){
	if(i==0){
	    minV=maxV=\$12
	    minR=maxR=\$15
	}
	else {
	    if(minV>\$12) minV=\$12
	    if(maxV<\$12) maxV=\$12
	    if(minR>\$15) minR=\$15
	    if(maxR<\$15) maxR=\$15
	}
	sumV += \$12
	sumR += \$15
	i++;
    }
}
END{
    meanV=sumV/(i*1.)
    meanR=sumR/(i*1.)
    printf "%d %.1f %d %d %.1f %d\n",minV,meanV,maxV,minR,meanR,maxR
}

EOF
        fi

        tmpfile2="$tmpfile.2"
        rm -f "$tmpfile2"
        if [ "${NUM_CORES}" == "" ]; then
            NUM_CORES=2
        fi
        for((i=1;i<NUM_CORES;i++)); do

	    grep "($i, " "$tmpfile" | tail -1 >> "$tmpfile2"

        done

        results=$(awk -f "$awkfile" "$tmpfile2")
        PEAK_VM_WORKERS=$(echo "$results" | awk '{print $3}')
        PEAK_RSS_WORKERS=$(echo "$results" | awk '{print $6}')
        AVE_VM_WORKERS=$(echo "$results" | awk '{print $2}')
        AVE_RSS_WORKERS=$(echo "$results" | awk '{print $5}')

    fi


}
