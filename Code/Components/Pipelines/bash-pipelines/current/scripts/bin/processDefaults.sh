#!/bin/bash -l
#
# This file takes the default values, after any modification by a
# user's config file, and creates other variables that depend upon
# them and do not require user input.
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

# Make sure we only run this file once!
if [ "$PROCESS_DEFAULTS_HAS_RUN" != "true" ]; then

    PROCESS_DEFAULTS_HAS_RUN=true

    ####################
    # Set the overall switches for CAL & SCIENCE fields

    if [ "${DO_1934_CAL}" != "true" ]; then
        # turn off all calibrator-related switches
        DO_SPLIT_1934=false
        DO_FLAG_1934=false
        DO_FIND_BANDPASS=false
    fi

    if [ "${DO_SCIENCE_FIELD}" != "true" ]; then
        # turn off all science-field switches
        DO_SPLIT_SCIENCE=false
        DO_FLAG_SCIENCE=false
        DO_APPLY_BANDPASS=false
        DO_AVERAGE_CHANNELS=false
        DO_CONT_IMAGING=false
        DO_SELFCAL=false
        DO_APPLY_CAL_CONT=false
        DO_APPLY_CAL_SL=false
        DO_CONTCUBE_IMAGING=false
        DO_SPECTRAL_IMAGING=false
        DO_SPECTRAL_IMSUB=false
        DO_MOSAIC=true
        DO_SOURCE_FINDING_CONT=false
        DO_SOURCE_FINDING_SPEC=false
        DO_SOURCE_FINDING_BEAMWISE=false
        DO_ALT_IMAGER=false
        #
        DO_CONVERT_TO_FITS=false
        DO_MAKE_THUMBNAILS=false
        DO_STAGE_FOR_CASDA=false
    fi

    # Turn off the purging of the full-resolution MS if we need to use it.
    if [ "${DO_COPY_SL}" == "true" ] ||
           [ "${DO_APPLY_CAL_SL}" == "true" ] ||
           [ "${DO_CONT_SUB_SL}" == "true" ] ||
           [ "${DO_SPECTRAL_IMAGING}" == "true" ]; then

        PURGE_FULL_MS=false

    fi

    ####################
    # Define the full path of output directory
    OUTPUT="${BASEDIR}/${OUTPUT}"
    ORIGINAL_OUTPUT="${OUTPUT}"
    mkdir -p "$OUTPUT"
    . "${PIPELINEDIR}/utils.sh"
    BASEDIR=${BASEDIR}
    cd "$OUTPUT"
    echo "$NOW" >> PROCESSED_ON
    if [ ! -e "${stats}" ]; then
        ln -s "${BASEDIR}/${stats}" .
    fi
    cd "$BASEDIR"

    ####################
    # ASKAPsoft module usage

    # Make use of the ASKAP module directory right now
    moduleDir="/group/askap/modulefiles"
    module use "$moduleDir"

    if [ "${ASKAP_ROOT}" == "" ]; then
        # Has the user asked for a specific askapsoft module?
        if [ "${ASKAPSOFT_VERSION}" != "" ]; then
            # If so, ensure it exists
            if [ "$(module avail -t "askapsoft/${ASKAPSOFT_VERSION}" 2>&1 | grep askapsoft)" == "" ]; then
                echo "WARNING - Requested askapsoft version ${ASKAPSOFT_VERSION} not available"
                ASKAPSOFT_VERSION=""
            else
                # It exists. Add a leading slash so we can append to 'askapsoft' in the module call
                ASKAPSOFT_VERSION="/${ASKAPSOFT_VERSION}"
            fi
        fi
        # load the modules correctly
        currentASKAPsoftVersion="$(module list -t 2>&1 | grep askapsoft)"
        if [ "${currentASKAPsoftVersion}" == "" ]; then
            # askapsoft is not loaded by the .bashrc - need to specify for
            # slurm jobfiles. Use the requested one if necessary.
            if [ "${ASKAPSOFT_VERSION}" == "" ]; then
                askapsoftModuleCommands="# Loading the default askapsoft module"
                echo "Will use the default askapsoft module"
            else
                askapsoftModuleCommands="# Loading the requested askapsoft module"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
            fi
            askapsoftModuleCommands="${askapsoftModuleCommands}
module use $moduleDir
module load askapdata
module load askapsoft${ASKAPSOFT_VERSION}"
            module load askapsoft${ASKAPSOFT_VERSION}
        else
            # askapsoft is currently available in the user's module list
            #  If a specific version has been requested, swap to that
            #  Otherwise, do nothing
            if [ "${ASKAPSOFT_VERSION}" != "" ]; then
                askapsoftModuleCommands="# Swapping to the requested askapsoft module
module use $moduleDir
module load askapdata
module swap askapsoft askapsoft${ASKAPSOFT_VERSION}"
                echo "Will use the askapsoft module askapsoft${ASKAPSOFT_VERSION}"
                module swap askapsoft askapsoft${ASKAPSOFT_VERSION}
            else
                askapsoftModuleCommands="# Using user-defined askapsoft module
module use $moduleDir
module load askapdata
module unload askapsoft
module load ${currentASKAPsoftVersion}"
                echo "Will use the askapsoft module defined by your environment (${currentASKAPsoftVersion})"
            fi
        fi

        # askappipeline module
        askappipelineVersion=$(module list -t 2>&1 | grep askappipeline | sed -e 's|askappipeline/||g')
        askapsoftModuleCommands="$askapsoftModuleCommands
module unload askappipeline
module load askappipeline/${askappipelineVersion}"

        echo " "

    else
        askapsoftModuleCommands="# Using ASKAPsoft code tree directly, so no need to load modules"
        echo "Using ASKAPsoft code direct from your code tree at ASKAP_ROOT=$ASKAP_ROOT"
        echo "ASKAPsoft modules will *not* be loaded in the slurm files."
    fi

    #############################
    # CONVERSION TO FITS FORMAT

    # This function returns a bunch of text in $fitsConvertText that can
    # be pasted into an external slurm job file.
    # The text will perform the conversion of an given CASA image to FITS
    # format, and update the headers and history appropriately.
    # For the most recent askapsoft versions, it will use the imageToFITS
    # tool to do both, otherwise it will use casa to do so.

    # Check whether imageToFITS is defined in askapsoft module being
    # used
    if [ "$(which imageToFITS 2> "${tmp}/whchim2fts")" == "" ]; then
        # Not found - use casa to do conversion
        fitsConvertText="# The following converts the file in \$casaim to a FITS file, after fixing headers.
if [ -e \"\${casaim}\" ] && [ ! -e \"\${fitsim}\" ]; then
    # The FITS version of this image doesn't exist

    script=\$(echo \"\${parset}\" | sed -e 's/\.in/\.py/g')
    ASKAPSOFT_VERSION=\"${ASKAPSOFT_VERSION}\"
    if [ \"\${ASKAPSOFT_VERSION}\" == \"\" ]; then
        ASKAPSOFT_VERSION_USED=\$(module list -t 2>&1 | grep askapsoft)
    else
        ASKAPSOFT_VERSION_USED=\$(echo \"\${ASKAPSOFT_VERSION}\" | sed -e 's|/||g')
    fi

    cat > \$script << EOFSCRIPT
#!/usr/bin/env python

casaimage='\${casaim}'
fitsimage='\${fitsim}'

ia.open(casaimage)
info=ia.miscinfo()
info['PROJECT']='${PROJECT_ID}'
info['DATE-OBS']='${DATE_OBS}'
info['DURATION']=${DURATION}
info['SBID']='${SB_SCIENCE}'
#info['INSTRUME']='${INSTRUMENT}'
ia.setmiscinfo(info)
imhistory=[]
imhistory.append('Produced with ASKAPsoft version \${ASKAPSOFT_VERSION_USED}')
imhistory.append('Produced using ASKAP pipeline version ${PIPELINE_VERSION}')
imhistory.append('Processed with ASKAP pipelines on ${NOW_FMT}')
ia.sethistory(origin='ASKAPsoft pipeline',history=imhistory)
ia.tofits(outfile=fitsimage, velocity=False)
ia.close()
EOFSCRIPT

    NCORES=1
    NPPN=1
    module load casa
    aprun -n \${NCORES} -N \${NPPN} -b casa --nogui --nologger --log2term -c \"\${script}\" >> \"\${log}\"

fi"
    else
        # We can use the new imageToFITS utility to do the conversion.
        fitsConvertText="# The following converts the file in \$casaim to a FITS file, after fixing headers.
if [ -e \"\${casaim}\" ] && [ ! -e \"\${fitsim}\" ]; then
    # The FITS version of this image doesn't exist

    ASKAPSOFT_VERSION=\"\${ASKAPSOFT_VERSION}\"
    if [ \"\${ASKAPSOFT_VERSION}\" == \"\" ]; then
        ASKAPSOFT_VERSION_USED=\$(module list -t 2>&1 | grep askapsoft)
    else
        ASKAPSOFT_VERSION_USED=\$(echo \"\${ASKAPSOFT_VERSION}\" | sed -e 's|/||g')
    fi

    cat > \"\$parset\" << EOFINNER
ImageToFITS.casaimage = \${casaim}
ImageToFITS.fitsimage = \${fitsim}
ImageToFITS.stokesLast = true
ImageToFITS.headers = [\"project\", \"sbid\", \"date-obs\", \"duration\"]
ImageToFITS.headers.project = ${PROJECT_ID}
ImageToFITS.headers.sbid = ${SB_SCIENCE}
ImageToFITS.headers.date-obs = ${DATE_OBS}
ImageToFITS.headers.duration = ${DURATION}
ImageToFITS.history = [\"Produced with ASKAPsoft version \${ASKAPSOFT_VERSION_USED}\", \"Produced using ASKAP pipeline version ${PIPELINE_VERSION}\", \"Processed with ASKAP pipelines on ${NOW_FMT}\"]
EOFINNER
    NCORES=1
    NPPN=1
    aprun -n \${NCORES} -N \${NPPN} imageToFITS -c \"\${parset}\" >> \"\${log}\"

fi"
    fi


    ####################
    # Slurm file headers

    # Reservation string
    if [ "$RESERVATION" != "" ]; then
        RESERVATION_REQUEST="#SBATCH --reservation=${RESERVATION}"
    else
        RESERVATION_REQUEST="# No reservation requested"
    fi

    # Email request
    if [ "$EMAIL" != "" ]; then
        EMAIL_REQUEST="#SBATCH --mail-user=${EMAIL}
#SBATCH --mail-type=${EMAIL_TYPE}"
    else
        EMAIL_REQUEST="# No email notifications sent"
    fi

    # Account to be used
    if [ "$ACCOUNT" != "" ]; then
        ACCOUNT_REQUEST="#SBATCH --account=${ACCOUNT}"
    else
        ACCOUNT_REQUEST="# Using the default account"
    fi

    ####################
    # Set the times for each job
    if [ "$JOB_TIME_SPLIT_1934" == "" ]; then
        JOB_TIME_SPLIT_1934=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPLIT_SCIENCE" == "" ]; then
        JOB_TIME_SPLIT_SCIENCE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FLAG_1934" == "" ]; then
        JOB_TIME_FLAG_1934=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FLAG_SCIENCE" == "" ]; then
        JOB_TIME_FLAG_SCIENCE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FIND_BANDPASS" == "" ]; then
        JOB_TIME_FIND_BANDPASS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_APPLY_BANDPASS" == "" ]; then
        JOB_TIME_APPLY_BANDPASS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_AVERAGE_MS" == "" ]; then
        JOB_TIME_AVERAGE_MS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_IMAGE" == "" ]; then
        JOB_TIME_CONT_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONT_APPLYCAL" == "" ]; then
        JOB_TIME_CONT_APPLYCAL=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CONTCUBE_IMAGE" == "" ]; then
        JOB_TIME_CONTCUBE_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_SPLIT" == "" ]; then
        JOB_TIME_SPECTRAL_SPLIT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_APPLYCAL" == "" ]; then
        JOB_TIME_SPECTRAL_APPLYCAL=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_CONTSUB" == "" ]; then
        JOB_TIME_SPECTRAL_CONTSUB=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_IMAGE" == "" ]; then
        JOB_TIME_SPECTRAL_IMAGE=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SPECTRAL_IMCONTSUB" == "" ]; then
        JOB_TIME_SPECTRAL_IMCONTSUB=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_LINMOS" == "" ]; then
        JOB_TIME_LINMOS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SOURCEFINDING_CONT" == "" ]; then
        JOB_TIME_SOURCEFINDING_CONT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_SOURCEFINDING_SPEC" == "" ]; then
        JOB_TIME_SOURCEFINDING_SPEC=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_DIAGNOSTICS" == "" ]; then
        JOB_TIME_DIAGNOSTICS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_FITS_CONVERT" == "" ]; then
        JOB_TIME_FITS_CONVERT=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_THUMBNAILS" == "" ]; then
        JOB_TIME_THUMBNAILS=${JOB_TIME_DEFAULT}
    fi
    if [ "$JOB_TIME_CASDA_UPLOAD" == "" ]; then
        JOB_TIME_CASDA_UPLOAD=${JOB_TIME_DEFAULT}
    fi

    ####################
    # Configure the list of beams to be processed
    # Lists each beam in ${BEAMS_TO_USE}

    BEAMS_TO_USE=""
    if [ "${BEAMLIST}" == "" ]; then
        # just use BEAM_MIN & BEAM_MAX
        for((b=BEAM_MIN;b<=BEAM_MAX;b++)); do
            thisbeam=$(echo "$b" | awk '{printf "%02d",$1}')
            BEAMS_TO_USE="${BEAMS_TO_USE} $thisbeam"
        done
    else
        # re-print out the provided beam list with 0-leading integers
        beamAwkFile="$tmp/beamParsing.awk"
        cat > "$beamAwkFile" <<EOF
BEGIN{
  str=""
}
{
  n=split(\$1,a,",");
  for(i=1;i<=n;i++){
    n2=split(a[i],a2,"-");
    for(b=a2[1];b<=a2[n2];b++){
      str=sprintf("%s %02d",str,b);
    }
  }
}
END{
  print str
}
EOF
        BEAMS_TO_USE=$(echo "$BEAMLIST" | awk -f "$beamAwkFile")
    fi

    echo "Using the following beams for the science data: $BEAMS_TO_USE"

    # Check the number of beams, and the maximum beam number (need the
    # latter for calibration tasks)
    maxbeam=-1
    nbeams=0
    for b in ${BEAMS_TO_USE}; do
        thisbeam=$( echo "$b" | awk '{printf "%d",$1}')
        if [ "$thisbeam" -gt "$maxbeam" ]; then
            maxbeam=$thisbeam
        fi
        ((nbeams++))
    done
    ((maxbeam++))

    # Handle the case where the SB MS is missing.
    # First for the bandpass calibrator
    if [ "${MS_CAL_MISSING}" == "true" ]; then
        # Turn off splitting
        DO_SPLIT_1934=false
        # Check for existence of each MS
        for((IBEAM=0; IBEAM<=highestBeam; IBEAM++)); do
            BEAM=$(echo "$IBEAM" | awk '{printf "%02d",$1}')
            find1934MSnames
            if [ "${DO_1934_CAL}" == "true" ] && [ ! -e ${msCal} ]; then
                echo "The MS for the calibration SB does not exist, and at least one derived MS (${msCal}) is not present."
                echo "Turning off Bandpass Calibration processing ('DO_1934_CAL=false')"
                DO_1934_CAL=false
            fi
        done
    fi
    # Now for the science field
    if [ "${MS_SCIENCE_MISSING}" == "true" ]; then
        # Turn off splitting
        DO_SPLIT_SCIENCE=false
        # Check for existence of each MS
        for BEAM in ${BEAMS_TO_USE}; do
            findScienceMSnames
            if [ "${DO_SCIENCE_FIELD}" == "true" ]; then
                if [ ! -e ${msSci} ] && [ ! -e ${msSciAv} ]; then
                    echo "The MS for the science field SB does not exist, and at least one beam (${BEAM}) does not have derived MSs."
                    echo "Turning off Science Field processing ('DO_SCIENCE_FIELD=false')"
                    DO_SCIENCE_FIELD=false
                fi
            fi
        done
    fi

    echo " "

    ####################
    # Parameters required for science field imaging
    ####

    if [ "${DO_SCIENCE_FIELD}" == "true" ]; then

        if [ "${NSUB_CUBES}" != "" ]; then
            echo "WARNING - the parameter NSUB_CUBES is deprectated. Using NUM_SPECTRAL_CUBES=${NUM_SPECTRAL_CUBES} instead."
        fi

        ####################
        # Parameters required for continuum imaging
        ####

        # Check value of IMAGETYPE - needs to be casa or fits
        if [ "${IMAGETYPE_CONT}" != "casa" ] && [ "${IMAGETYPE_CONT}" != "fits" ]; then
            echo "ERROR - Invalid image type \"${IMAGETYPE_CONT}\" - IMAGETYPE_CONT needs to be casa or fits"
            echo "   Exiting"
            exit 1
        fi

        # Switching on the DO_ALT_IMAGER_CONT flag - if it isn't
        # defined in the config file, then set to the value of
        # DO_ALT_IMAGER. 
        if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_CONT}" == "" ]; then
            echo "WARNING - You have not defined DO_ALT_IMAGER_CONT - setting to $DO_ALT_IMAGER, the value of DO_ALT_IMAGER"
            DO_ALT_IMAGER_CONT=${DO_ALT_IMAGER}
        fi

        # Total number of channels must be exact multiple of averaging width.
        # If it isn't, report an error and exit without running anything.
        averageWidthOK=$(echo "${NUM_CHAN_SCIENCE}" "${NUM_CHAN_TO_AVERAGE}" | awk '{if (($1 % $2)==0) print "yes"; else print "no"}')
        if [ "${averageWidthOK}" == "no" ]; then
            echo "ERROR! Number of channels (${NUM_CHAN_SCIENCE}) must be an exact multiple of NUM_CHAN_TO_AVERAGE (${NUM_CHAN_TO_AVERAGE}). Exiting."
            exit 1
        fi

        # nchanContSci = number of channels after averaging
        nchanContSci=$(echo "${NUM_CHAN_SCIENCE}" "${NUM_CHAN_TO_AVERAGE}" | awk '{print $1/$2}')

        # nworkergroupsSci = number of worker groups, used for MFS imaging.
        nworkergroupsSci=$(echo "${NUM_TAYLOR_TERMS}" | awk '{print 2*$1-1}')

        # total number of CPUs required for MFS continuum imaging, including the master
        #  Use the number given in the config file, unless it has been left blank
        if [ "${NUM_CPUS_CONTIMG_SCI}" == "" ]; then
            NUM_CPUS_CONTIMG_SCI=$(echo "$nchanContSci" "$nworkergroupsSci" | awk '{print $1*$2+1}')
        fi

        # if we are using the new imager we need to tweak this
        if [ "${DO_ALT_IMAGER_CONT}" == "true" ]; then
            NUM_CPUS_CONTIMG_SCI=$(echo "$nchanContSci" "$nworkergroupsSci" "${NCHAN_PER_CORE}" | awk '{print ($1/$3)*$2+1}')
            # CPUS_PER_CORE_CONT_IMAGING=8 # get rid of this change as it is unnecessary
        fi

        # Can't have -N greater than -n in the aprun call
        if [ "${NUM_CPUS_CONTIMG_SCI}" -lt "${CPUS_PER_CORE_CONT_IMAGING}" ]; then
            CPUS_PER_CORE_CONT_IMAGING=${NUM_CPUS_CONTIMG_SCI}
        fi

        # Method used for self-calibration - needs to be either Cmodel or Components
        if [ "${SELFCAL_METHOD}" != "Cmodel" ] &&
               [ "${SELFCAL_METHOD}" != "Components" ]; then
            SELFCAL_METHOD="Cmodel"
        fi



        ####################
        # Parameters required for continuum-cube imaging
        ####

        # Check value of IMAGETYPE - needs to be casa or fits
        if [ "${IMAGETYPE_CONTCUBE}" != "casa" ] && [ "${IMAGETYPE_CONTCUBE}" != "fits" ]; then
            echo "ERROR - Invalid image type \"${IMAGETYPE_CONTCUBE}\" - IMAGETYPE_CONTCUBE needs to be casa or fits"
            echo "   Exiting"
            exit 1
        fi

        # Switching on the DO_ALT_IMAGER_CONTCUBE flag - if it isn't
        # defined in the config file, then set to the value of
        # DO_ALT_IMAGER. 
        if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_CONTCUBE}" == "" ]; then
            echo "WARNING - You have not defined DO_ALT_IMAGER_CONTCUBE - setting to $DO_ALT_IMAGER, the value of DO_ALT_IMAGER"
            DO_ALT_IMAGER_CONTCUBE=${DO_ALT_IMAGER}
        fi

        # simager is not currently able to write out FITS files. So if
        # the user has requested FITS imagetype, but has not set the
        # ALT_IMAGER flag, give a warning and stop to let them fix it.
        if [ "${IMAGETYPE_CONTCUBE}" == "fits" ] && [ "${DO_ALT_IMAGER_CONTCUBE}" != "true" ]; then
            echo "ERROR - IMAGETYPE_CONTCUBE=fits can only work with DO_ALT_IMAGER_CONTCUBE=true"
            echo "   Exiting"
            exit 1
        fi

        # Set the polarisation list for the continuum cubes
        if [ "${CONTCUBE_POLARISATIONS}" == "" ]; then
            if [ "$DO_CONTCUBE_IMAGING" == "true" ]; then
                echo "WARNING - No polarisation given for continuum cube imaging. Turning off DO_CONTCUBE_IMAGING"
                DO_CONTCUBE_IMAGING=false
            fi
        else
            # set the POL_LIST parameter
            getPolList
        fi

        # Set the number of cores for the continuum cube imaging. Either
        # set to the number of averaged channels + 1, or use that given in
        # the config file, limiting to no bigger than this number
        maxContCubeCores=$((nchanContSci + 1))
        if [ "${NUM_CPUS_CONTCUBE_SCI}" == "" ]; then
            # User has not specified
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
        elif [ "${NUM_CPUS_CONTCUBE_SCI}" -gt "$maxContCubeCores" ]; then
            # Have more cores than we need - reduce number
            echo "NOTE - Reducing NUM_CPUS_CONTCUBE_SCI to $maxContCubeCores to match the number of averaged channels"
            NUM_CPUS_CONTCUBE_SCI=$maxContCubeCores
        fi
        if [ "${CPUS_PER_CORE_CONTCUBE_IMAGING}" -gt 20 ]; then
            CPUS_PER_CORE_CONTCUBE_IMAGING=20
        fi
        if [ "${CPUS_PER_CORE_CONTCUBE_IMAGING}" -gt "${NUM_CPUS_CONTCUBE_SCI}" ]; then
            CPUS_PER_CORE_CONTCUBE_IMAGING=${NUM_CPUS_CONTCUBE_SCI}
        fi

        # Set the number of cores for the continuum cube mosaicking. Either
        # set to the number of averaged channels, or use that given in
        # the config file
        if [ "${NUM_CPUS_CONTCUBE_LINMOS}" == "" ]; then
            # User has not specified
            NUM_CPUS_CONTCUBE_LINMOS=$nchanContSci
        elif [ "${NUM_CPUS_CONTCUBE_LINMOS}" -gt "${NUM_CPUS_CONTCUBE_SCI}" ]; then
            echo "NOTE - Reducing NUM_CPUS_CONTCUBE_LINMOS to $NUM_CPUS_CONTCUBE_SCI to match the number of cores used for imaging"
            NUM_CPUS_CONTCUBE_LINMOS=$NUM_CPUS_CONTCUBE_SCI
        fi

        # Define the list of writer ranks used in the askap_imager
        # spectral-line output
        # Only define if we are using the askap_imager and not writing
        # to a single file. Otherwise, we define a single-value list
        # so that the loop over subbands is only done once ($subband
        # will not be referenced in that case).
        if [ "${DO_ALT_IMAGER_CONTCUBE}" == "true" ] && [ "${ALT_IMAGER_SINGLE_FILE_CONTCUBE}" != "true" ]; then
            nworkers=$nchanContSci
            writerIncrement=$(echo "$nworkers" "${NUM_SPECTRAL_CUBES_CONTCUBE}" | awk '{print $1/$2}')
            SUBBAND_WRITER_LIST_CONTCUBE=$(seq 1 "$writerIncrement" "$nworkers")
            unset nworkers
            unset writerIncrement
        else
            SUBBAND_WRITER_LIST_CONTCUBE=1
            NUM_SPECTRAL_CUBES_CONTCUBE=1
        fi


        ####################
        # Parameters required for spectral-line imaging
        ####

        # Check value of IMAGETYPE - needs to be casa or fits
        if [ "${IMAGETYPE_SPECTRAL}" != "casa" ] && [ "${IMAGETYPE_SPECTRAL}" != "fits" ]; then
            echo "ERROR - Invalid image type \"${IMAGETYPE_SPECTRAL}\" - IMAGETYPE_SPECTRAL needs to be casa or fits"
            echo "   Exiting"
            exit 1
        fi

        # Switching on the DO_ALT_IMAGER_SPECTRAL flag - if it isn't
        # defined in the config file, then set to the value of
        # DO_ALT_IMAGER.  
        if [ "${DO_ALT_IMAGER}" == "true" ] && [ "${DO_ALT_IMAGER_SPECTRAL}" == "" ]; then
            echo "WARNING - You have not defined DO_ALT_IMAGER_SPECTRAL - setting to $DO_ALT_IMAGER, the value of DO_ALT_IMAGER"
            DO_ALT_IMAGER_SPECTRAL=${DO_ALT_IMAGER}
        fi

        # simager is not currently able to write out FITS files. So if
        # the user has requested FITS imagetype, but has not set the
        # ALT_IMAGER flag, give a warning and stop to let them fix it.
        if [ "${IMAGETYPE_SPECTRAL}" == "fits" ] && [ "${DO_ALT_IMAGER_SPECTRAL}" != "true" ]; then
            echo "ERROR - IMAGETYPE_SPECTRAL=fits can only work with DO_ALT_IMAGER_SPECTRAL=true"
            echo "   Exiting"
            exit 1
        fi

        # Channel range to be used for spectral-line imaging
        # If user has requested a different channel range, and the
        # DO_COPY_SL flag is set, then we define a new spectral-line
        # range of channels
        if [ "${CHAN_RANGE_SL_SCIENCE}" == "" ] ||
               [ "${DO_COPY_SL}" != "true" ]; then
            # If this range hasn't been set, or we aren't doing the
            # copy, then use the full channel range
            CHAN_RANGE_SL_SCIENCE="1-$NUM_CHAN_SCIENCE"
        fi
        # Check that we haven't requested invalid channels
        minSLchan=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $1}')
        maxSLchan=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $2}')
        if [ "$minSLchan" -lt 0 ] || [ "$minSLchan" -gt "${NUM_CHAN_SCIENCE}" ] ||
               [ "$maxSLchan" -lt 0 ] || [ "$maxSLchan" -gt "${NUM_CHAN_SCIENCE}" ] ||
               [ "$minSLchan" -gt "$maxSLchan" ]; then
            echo "ERROR - Invalid selection of CHAN_RANGE_SL_SCIENCE=$CHAN_RANGE_SL_SCIENCE, for total number of channels = $NUM_CHAN_SCIENCE"
            echo "   Exiting."
            exit 1
        fi
        # Define the number of channels used in the spectral-line imaging
        NUM_CHAN_SCIENCE_SL=$(echo "${CHAN_RANGE_SL_SCIENCE}" | awk -F'-' '{print $2-$1+1}')

        # if we are using the new imager we need to tweak this
        if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ]; then
            NUM_CPUS_SPECIMG_SCI=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NCHAN_PER_CORE_SL}" | awk '{print ($1/$2) + 1}')
            # CPUS_PER_CORE_SPEC_IMAGING=16 # get rid of this change as it is unnecessary
        fi

        # Can't have -N greater than -n in the aprun call
        if [ "${NUM_CPUS_SPECIMG_SCI}" -lt "${CPUS_PER_CORE_SPEC_IMAGING}" ]; then
            CPUS_PER_CORE_SPEC_IMAGING=${NUM_CPUS_SPECIMG_SCI}
        fi

        # Method used for continuum subtraction
        if [ "${CONTSUB_METHOD}" != "Cmodel" ] &&
               [ "${CONTSUB_METHOD}" != "Components" ] &&
               [ "${CONTSUB_METHOD}" != "CleanModel" ]; then
            CONTSUB_METHOD="Cmodel"
        fi
        # Old way of choosing above
        if [ "${BUILD_MODEL_FOR_CONTSUB}" != "" ] &&
               [ "${BUILD_MODEL_FOR_CONTSUB}" != "true" ]; then
            echo "WARNING - the parameter BUILD_MODEL_FOR_CONTSUB is deprecated - please use CONTSUB_METHOD instead"
            CONTSUB_METHOD="Cmodel"
        fi

        if [ "${RESTORING_BEAM_LOG}" != "" ]; then
            echo "WARNING - the parameter RESTORING_BEAM_LOG is deprecated, and is constructed from the image name instead."
        fi

        # Define the list of writer ranks used in the askap_imager
        # spectral-line output
        # Only define if we are using the askap_imager and not writing
        # to a single file. Otherwise, we define a single-value list
        # so that the loop over subbands is only done once ($subband
        # will not be referenced in that case).
        if [ "${DO_ALT_IMAGER_SPECTRAL}" == "true" ] && [ "${ALT_IMAGER_SINGLE_FILE}" != "true" ]; then
            nworkers=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NCHAN_PER_CORE_SL}" | awk '{print $1/$2}')
            writerIncrement=$(echo "$nworkers" "${NUM_SPECTRAL_CUBES}" | awk '{print $1/$2}')
            SUBBAND_WRITER_LIST=$(seq 1 "$writerIncrement" "$nworkers")
            unset nworkers
            unset writerIncrement
        else
            SUBBAND_WRITER_LIST=1
            NUM_SPECTRAL_CUBES=1
        fi

        ####################
        # Mosaicking parameters

        # Fix the direction string for linmos - don't need the J2000 bit
        linmosFeedCentre=$(echo "${DIRECTION_SCI}" | awk -F',' '{printf "%s,%s]",$1,$2}')

        # Total number of channels should be exact multiple of
        # channels-per-core, else the final process will take care of
        # the rest and may run out of memory
        # If it isn't, give a warning and push on
        chanPerCoreLinmosOK=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NUM_SPECTRAL_CUBES}" "${NCHAN_PER_CORE_SPECTRAL_LINMOS}" | awk '{if (($1/$2 % $3)==0) print "yes"; else print "no"}')
        if [ "${chanPerCoreLinmosOK}" == "no" ] && [ "${DO_MOSAIC}" == "true" ]; then
            echo "Warning! Number of spectral-line channels (${NUM_CHAN_SCIENCE_SL}) is not an exact multiple of NCHAN_PER_CORE_SPECTRAL_LINMOS (${NCHAN_PER_CORE_SPECTRAL_LINMOS})."
            echo "         Pushing on, but there is the risk of memory problems with the spectral linmos task."
        fi
        # Determine the number of cores needed for spectral-line mosaicking
        if [ "$NUM_CPUS_SPECTRAL_LINMOS" == "" ]; then
            NUM_CPUS_SPECTRAL_LINMOS=$(echo "${NUM_CHAN_SCIENCE_SL}" "${NUM_SPECTRAL_CUBES}" "${NCHAN_PER_CORE_SPECTRAL_LINMOS}" | awk '{if($1%($2*$3)==0) print $1/$2/$3; else print int($1/$2/$3)+1;}')
        fi

        ####################
        # Source finding switches

        # First check for the old DO_SOURCE_FINDING.
        #  If present, set new source-finding switches to its value
        #  and give a warning
        if [ "${DO_SOURCE_FINDING}" != "" ]; then
            DO_SOURCE_FINDING_CONT=${DO_SOURCE_FINDING}
            DO_SOURCE_FINDING_SPEC=${DO_SOURCE_FINDING}
            echo "WARNING - Parameter DO_SOURCE_FINDING has been deprecated."
            echo "        - Please use DO_SOURCE_FINDING_CONT and DO_SOURCE_FINDING_SPEC."
            echo "        - For now, these have both been set to ${DO_SOURCE_FINDING}."
        fi

        # If DO_SOURCE_FINDING_CONT has not been set, set it to be the same as the DO_CONT_IMAGING switch
        if [ "${DO_SOURCE_FINDING_CONT}" == "" ]; then
            DO_SOURCE_FINDING_CONT=${DO_CONT_IMAGING}
        fi

        # If DO_SOURCE_FINDING_SPEC has not been set, set it to be the same as the DO_SPECTRAL_IMAGING switch
        if [ "${DO_SOURCE_FINDING_SPEC}" == "" ]; then
            DO_SOURCE_FINDING_SPEC=${DO_SPECTRAL_IMAGING}
        fi

        ####################
        # Continuum Source-finding

        # Number of cores:
        NUM_CPUS_SELAVY=$(echo "${SELAVY_NSUBX}" "${SELAVY_NSUBY}" | awk '{print $1*$2+1}')
        CPUS_PER_CORE_SELAVY=${NUM_CPUS_SELAVY}
        if [ "${CPUS_PER_CORE_SELAVY}" -gt 20 ]; then
            CPUS_PER_CORE_SELAVY=20
        fi
        if [ "${CPUS_PER_CORE_SELAVY}" -gt "${NUM_CPUS_SELAVY}" ]; then
            CPUS_PER_CORE_SELAVY=${NUM_CPUS_SELAVY}
        fi

        # If the sourcefinding flag has been set, but we aren't
        # mosaicking, turn on the beam-wise sourcefinding flag
        if [ "${DO_SOURCE_FINDING_CONT}" == "true" ] && [ "${DO_MOSAIC}" != "true" ]; then
            DO_SOURCE_FINDING_BEAMWISE=true
        fi

        if [ "${SELAVY_POL_WRITE_FDF}" != "" ]; then
            echo "WARNING - the parameter SELAVY_POL_WRITE_FDF is deprecated"
            echo "        - use SELAVY_POL_WRITE_COMPLEX_FDF instead (it has been set to ${SELAVY_POL_WRITE_FDF})"
            SELAVY_POL_WRITE_COMPLEX_FDF=${SELAVY_POL_WRITE_FDF}
        fi


        ####################
        # Spectral source-finding

        # Number of cores etc
        NUM_CPUS_SELAVY_SPEC=$(echo "${SELAVY_SPEC_NSUBX}" "${SELAVY_SPEC_NSUBY}" "${SELAVY_SPEC_NSUBZ}" | awk '{print $1*$2*$3+1}')
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" == "" ]; then
            CPUS_PER_CORE_SELAVY_SPEC=${NUM_CPUS_SELAVY_SPEC}
        fi
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" -gt 20 ]; then
            CPUS_PER_CORE_SELAVY_SPEC=20
        fi
        if [ "${CPUS_PER_CORE_SELAVY_SPEC}" -gt "${NUM_CPUS_SELAVY_SPEC}" ]; then
            CPUS_PER_CORE_SELAVY_SPEC=${NUM_CPUS_SELAVY_SPEC}
        fi

        # Check search type
        if [ "${SELAVY_SPEC_SEARCH_TYPE}" != "spectral" ] &&
               [ "${SELAVY_SPEC_SEARCH_TYPE}" != "spatial" ]; then
            SELAVY_SPEC_SEARCH_TYPE="spectral"
            echo "WARNING - SELAVY_SPEC_SEARCH_TYPE needs to be 'spectral' or 'spatial' - Setting to 'spectral'"
        fi

        # Check smooth type
        if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ]; then
            if [ "${SELAVY_SPEC_SMOOTH_TYPE}" != "spectral" ] ||
                   [ "${SELAVY_SPEC_SMOOTH_TYPE}" != "spatial" ]; then
                SELAVY_SPEC_SMOOTH_TYPE="spectral"
                echo "WARNING - SELAVY_SPEC_SMOOTH_TYPE needs to be 'spectral' or 'spatial' - Setting to 'spectral'"
            fi
        fi

        # Spatial smoothing kernel - interpret to form selavy parameters
        if [ "${SELAVY_SPEC_FLAG_SMOOTH}" == "true" ] &&
               [ "${SELAVY_SPEC_SMOOTH_TYPE}" == "spatial" ]; then
            kernelArray=()
            for a in $(echo "${SELAVY_SPEC_SPATIAL_KERNEL}" | sed -e 's/[][,]/ /g'); do
                kernelArray+=($a)
            done
            if [ "${#kernelArray[@]}" -eq 1 ]; then
                SELAVY_SPEC_KERN_MAJ=${kernelArray[0]}
                SELAVY_SPEC_KERN_MIN=${kernelArray[0]}
                SELAVY_SPEC_KERN_PA=0
            elif [ "${#kernelArray[@]}" -eq 3 ]; then
                SELAVY_SPEC_KERN_MAJ=${kernelArray[0]}
                SELAVY_SPEC_KERN_MIN=${kernelArray[1]}
                SELAVY_SPEC_KERN_PA=${kernelArray[2]}
            else
                echo "WARNING - badly formed parameter SELAVY_SPEC_SPATIAL_KERNEL=${SELAVY_SPEC_SPATIAL_KERNEL}"
                echo "        - turning off Selavy's spatial smoothing."
                SELAVY_SPEC_FLAG_SMOOTH=false
            fi
        fi


        ####################
        # Variable inputs to Continuum imaging and Self-calibration settings
        #  This section takes the provided parameters and creates
        #  arrays that have a (potentially) different value for each
        #  loop of the self-cal operation.
        #  Parameters covered are the selfcal interval, the
        #  source-finding threshold, and whether normalise gains is on
        #  or not
        if [ "${DO_CONT_IMAGING}" == "true" ]; then

            expectedArrSize=$((SELFCAL_NUM_LOOPS + 1))

            if [ "${DO_SELFCAL}" == "true" ]; then

                if [ "$(echo "${SELFCAL_INTERVAL}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_INTERVAL}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_INTERVAL ($SELFCAL_INTERVAL) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_INTERVAL_ARRAY=()
                    for a in $(echo "$SELFCAL_INTERVAL" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_INTERVAL_ARRAY+=($a)
                    done
                else
                    SELFCAL_INTERVAL_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_INTERVAL_ARRAY+=($SELFCAL_INTERVAL)
                    done
                fi

                if [ "$(echo "${SELFCAL_SELAVY_THRESHOLD}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_SELAVY_THRESHOLD}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_SELAVY_THRESHOLD ($SELFCAL_SELAVY_THRESHOLD) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_SELAVY_THRESHOLD_ARRAY=()
                    for a in $(echo "${SELFCAL_SELAVY_THRESHOLD}" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_SELAVY_THRESHOLD_ARRAY+=($a)
                    done
                else
                    SELFCAL_SELAVY_THRESHOLD_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_SELAVY_THRESHOLD_ARRAY+=($SELFCAL_SELAVY_THRESHOLD)
                    done
                fi

                if [ "$(echo "${SELFCAL_NORMALISE_GAINS}" | grep "\[")" != "" ]; then
                    # Have entered a comma-separate array in square brackets
                    arrSize=$(echo "${SELFCAL_NORMALISE_GAINS}" | sed -e 's/[][,]/ /g' | wc -w)
                    if [ "$arrSize" -ne "$expectedArrSize" ]; then
                        echo "ERROR - SELFCAL_NORMALISE_GAINS ($SELFCAL_NORMALISE_GAINS) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                        exit 1
                    fi
                    SELFCAL_NORMALISE_GAINS_ARRAY=()
                    for a in $(echo "${SELFCAL_NORMALISE_GAINS}" | sed -e 's/[][,]/ /g'); do
                        SELFCAL_NORMALISE_GAINS_ARRAY+=($a)
                    done
                else
                    SELFCAL_NORMALISE_GAINS_ARRAY=()
                    for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                        SELFCAL_NORMALISE_GAINS_ARRAY+=($SELFCAL_NORMALISE_GAINS)
                    done
                fi

            fi

            if [ "$(echo "${CLEAN_NUM_MAJORCYCLES}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_NUM_MAJORCYCLES}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_NUM_MAJORCYCLES ($CLEAN_NUM_MAJORCYCLES) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_NUM_MAJORCYCLES_ARRAY=()
                for a in $(echo "${CLEAN_NUM_MAJORCYCLES}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_NUM_MAJORCYCLES_ARRAY+=($a)
                done
            else
                CLEAN_NUM_MAJORCYCLES_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_NUM_MAJORCYCLES_ARRAY+=($CLEAN_NUM_MAJORCYCLES)
                done
            fi

            if [ "$(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CLEAN_THRESHOLD_MAJORCYCLE ($CLEAN_THRESHOLD_MAJORCYCLE) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=()
                for a in $(echo "${CLEAN_THRESHOLD_MAJORCYCLE}" | sed -e 's/[][,]/ /g'); do
                    CLEAN_THRESHOLD_MAJORCYCLE_ARRAY+=($a)
                done
            else
                CLEAN_THRESHOLD_MAJORCYCLE_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CLEAN_THRESHOLD_MAJORCYCLE_ARRAY+=($CLEAN_THRESHOLD_MAJORCYCLE)
                done
            fi

            if [ "$(echo "${CIMAGER_MINUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CIMAGER_MINUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CIMAGER_MINUV ($CIMAGER_MINUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CIMAGER_MINUV_ARRAY=()
                for a in $(echo "${CIMAGER_MINUV}" | sed -e 's/[][,]/ /g'); do
                    CIMAGER_MINUV_ARRAY+=($a)
                done
            else
                CIMAGER_MINUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CIMAGER_MINUV_ARRAY+=($CIMAGER_MINUV)
                done
            fi

            if [ "$(echo "${CCALIBRATOR_MINUV}" | grep "\[")" != "" ]; then
                # Have entered a comma-separate array in square brackets
                arrSize=$(echo "${CCALIBRATOR_MINUV}" | sed -e 's/[][,]/ /g' | wc -w)
                if [ "$arrSize" -ne "$expectedArrSize" ]; then
                    echo "ERROR - CCALIBRATOR_MINUV ($CCALIBRATOR_MINUV) needs to be of size $expectedArrSize (since SELFCAL_NUM_LOOPS=$SELFCAL_NUM_LOOPS)"
                    exit 1
                fi
                CCALIBRATOR_MINUV_ARRAY=()
                for a in $(echo "${CCALIBRATOR_MINUV}" | sed -e 's/[][,]/ /g'); do
                    CCALIBRATOR_MINUV_ARRAY+=($a)
                done
            else
                CCALIBRATOR_MINUV_ARRAY=()
                for((i=0;i<=SELFCAL_NUM_LOOPS;i++)); do
                    CCALIBRATOR_MINUV_ARRAY+=($CCALIBRATOR_MINUV)
                done
            fi

            # Validate that all these arrays are the same length as
            # SELFCAL_NUM_LOOPS, as long as the latter is >0
            if [ "${SELFCAL_NUM_LOOPS}" -gt 0 ]; then
                arraySize=$((SELFCAL_NUM_LOOPS + 1))

                if [ "${DO_SELFCAL}" == "true" ]; then
                    if [ "${#SELFCAL_INTERVAL_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_INTERVAL (${SELFCAL_INTERVAL}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                    if [ "${#SELFCAL_SELAVY_THRESHOLD_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_SELAVY_THRESHOLD (${SELFCAL_SELAVY_THRESHOLD}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                    if [ "${#SELFCAL_NORMALISE_GAINS_ARRAY[@]}" -ne "$arraySize" ]; then
                        echo "ERROR! Size of SELFCAL_NORMALISE_GAINS (${SELFCAL_NORMALISE_GAINS}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                        exit 1
                    fi
                fi

                if [ "${#CLEAN_NUM_MAJORCYCLES_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_NUM_MAJORCYCLES (${CLEAN_NUM_MAJORCYCLES}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CLEAN_THRESHOLD_MAJORCYCLE_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CLEAN_THRESHOLD_MAJORCYCLE (${CLEAN_THRESHOLD_MAJORCYCLE}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CIMAGER_MINUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CIMAGER_MINUV (${CIMAGER_MINUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi
                if [ "${#CCALIBRATOR_MINUV_ARRAY[@]}" -ne "$arraySize" ]; then
                    echo "ERROR! Size of CCALIBRATOR_MINUV (${CCALIBRATOR_MINUV}) needs to be SELFCAL_NUM_LOOPS + 1 ($arraySize). Exiting."
                    exit 1
                fi

            fi

        fi

    fi

fi
