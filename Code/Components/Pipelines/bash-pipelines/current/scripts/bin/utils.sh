#!/bin/bash -l
#
# This file holds various utility functions and environment variables
# that allow the scripts to do various things in a uniform manner.
#
# @copyright (c) 2015 CSIRO
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


##############################
# PIPELINE VERSION REPORTING

function reportVersion()
{

    echo "Running ASKAPsoft pipeline processing, version ${PIPELINE_VERSION}"

}

##############################
# ARCHIVING A CONFIG FILE

# Takes one argument, the config file
function archiveConfig()
{
    
    filename=$(basename "$1")
    extension="${filename##*.}"
    filename="${filename%.*}"
    archivedConfig=$slurmOut/${filename}__${NOW}.${extension}
    cp $1 $archivedConfig
    
}

##############################
# Rejuvenation

# Takes one argument, a file or directory
function rejuvenate()
{
    if [ "$1" != "" ]; then
        find $1 -exec touch {} \;
    fi
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
	ALL_JOB_IDS="$@"
    else
	ALL_JOB_IDS="${ALL_JOB_IDS},$@"
    fi
}

function recordJob()
{
    # Usage: recordJob ID "This is a long description of this job"
    addJobID $1
    echo "$1 -- $2" | tee -a ${JOBLIST}
}

# Function to add a job id to a list of dependencies. Calling syntax
# is:
#  DEP=`addDep "$DEP" "$ID"`
function addDep()
{
    DEP=$1
    if [ "$2" != "" ]; then
        if [ "$1" == "" ]; then
            DEP="-d afterok"
        fi
        DEP="$DEP:$2"
    fi
    echo $DEP
}

##############################
# FILENAME PARSING

# function to return the imageBase for continuum images for a beam, or
# for the linmos case if $BEAM=all
function setImageBaseCont()
{
    if [ ${BEAM} == "all" ]; then
        imageBase=${IMAGE_BASE_CONT}.linmos
    else
        imageBase=${IMAGE_BASE_CONT}.beam${BEAM}
    fi
}

# function to return the imageBase for spectral cubes for a beam, or
# for the linmos case if $BEAM=all
function setImageBaseSpectral()
{
    if [ ${BEAM} == "all" ]; then
        imageBase=${IMAGE_BASE_SPECTRAL}.linmos
    else
        imageBase=${IMAGE_BASE_SPECTRAL}.beam${BEAM}
    fi
}

# A function to work out the measurement set names for the
# full-resolution, spectral-line and channel-averaged cases, given the
# current BEAM.
function findScienceMSnames()
{
    
    # 1. Get the value for $msSci (the un-averaged MS)
    if [ "`echo ${MS_BASE_SCIENCE} | grep %b`" != "" ]; then
        # If we are here, then $MS_BASE_SCIENCE has a %b that needs to be
        # replaced by the current $BEAM value
        sedstr="s|%b|${BEAM}|g"
        msSci=`echo ${MS_BASE_SCIENCE} | sed -e $sedstr`
    else
        # If we are here, then there is no %b, and we just append
        # _${BEAM} to the MS name
        sedstr="s/\.ms/_${BEAM}\.ms/g"
        msSci=`echo ${MS_BASE_SCIENCE} | sed -e $sedstr`
    fi

    if [ ${DO_COPY_SL} == true ]; then
        # If we make a copy of the spectral-line MS, then append '_SL'
        # to the MS name before the suffix for the MS used for
        # spectral-line imaging
        sedstr="s/\.ms/_SL\.ms/g"
        msSciSL=`echo ${msSci} | sed -e $sedstr`
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
        msSciAv=`echo $msSci | sed -e $sedstr`
    else
        # If we are here, then the user has given a specific filename
        # for MS_SCIENCE_AVERAGE. In this case, we can either replace
        # the %b with the beam number, or leave as is (but give a
        # warning).
        if [ "`echo ${MS_SCIENCE_AVERAGE} | grep %b`" != "" ]; then
            # If we are here, then $MS_SCIENCE_AVERAGE has a %b that
            # needs to be replaced by the current $BEAM value
            sedstr="s|%b|${BEAM}|g"
            msSciAv=`echo ${MS_SCIENCE_AVERAGE} | sed -e $sedstr`
        else
            msSciAv=${MS_SCIENCE_AVERAGE}
            if [ $nbeam -gt 1 ]; then
                # Only give the warning if there is more than one beam
                # (which means we're using the same MS for them)
                echo "Warning! Using ${msSciAv} as averaged MS for beam ${BEAM}"
            fi
        fi

    fi

    if [ "${GAINS_CAL_TABLE}" == "" ]; then
        # The name of the gains cal table is blank, so turn off
        # selfcal & cal-apply for the SL case
        if [ ${DO_SELFCAL} == true ]; then
            DO_SELFCAL=false
            echo "Gains cal filename (GAINS_CAL_TABLE) blank, so turning off selfcal"
        fi
        if [ ${DO_APPLY_CAL_SL} == true ]; then
            DO_APPLY_CAL_SL=false
            echo "Gains cal filename (GAINS_CAL_TABlE) blank, so turning off SL cal apply"
        fi
    else
        # Otherwise, need to replace any %b with the current BEAM, if there is one present
        if [ "`echo ${GAINS_CAL_TABLE} | grep %b`" != "" ]; then
            # We have a %b that needs replacing
            sedstr="s|%b|${BEAM}|g"
            gainscaltab=`echo ${GAINS_CAL_TABLE} | sed -e $sedstr`
        else
            # just use filename as provided
            gainscaltab=${GAINS_CAL_TABLE}
        fi
    fi

}

function find1934MSnames()
{
    if [ "`echo ${MS_BASE_1934} | grep %b`" != "" ]; then
        # If we are here, then $MS_BASE_1934 has a %b that
        # needs to be replaced by the current $BEAM value
        sedstr="s|%b|${BEAM}|g"
        msCal=`echo ${MS_BASE_1934} | sed -e $sedstr`
    else
        msCal=${MS_BASE_1934}
        echo "Warning! Using ${msCal} as 1934-638 MS for beam ${BEAM}"
    fi

}

#############################
# CONVERSION TO FITS FORMAT

# This function returns a bunch of text in $fitsConvertText that can
# be pasted into an external slurm job file 

function convertToFITStext()
{

    fitsConvertText="# The following converts the file in \$casaim to a FITS file, then fixes headers.
if [ -e \${casaim} ] && [ ! -e \${fitsim} ]; then
    # The FITS version of this image doesn't exist

    aprun -n 1 $image2fits in=\${casaim} out=\${fitsim}

    script=$parsets/fixheader_\${casaim##*/}_\${SLURM_JOB_ID}.py
    log=$logs/fixheader_\${casaim##*/}_\${SLURM_JOB_ID}.log
    cat > \$script << EOFSCRIPT
#!/usr/bin/env python
import pyfits as fits
image='\${fitsim}'
project='${PROJECT_ID}'
hdulist = fits.open(image,'update')
hdulist[0].header.update('PROJECT',project)
hdulist.flush()
EOFSCRIPT

    aprun -n 1 python \$script > \$log

fi"

}

##############################
# BEAM FOOTPRINTS AND CENTRES

function setFootprintFile()
{
    # Function to define a file containing the beam locations for the
    # requested footprint, which is created for a given run and a given
    # field name.
    #  Required inputs:
    #     * NOW - date/time of current pipeline run
    #     * FIELD - name of field
    #  Returns: $footprintOut

    footprintOut="${parsets}/footprintOutput-${NOW}-${FIELD}.txt"
}

function getBeamOffsets()
{
    # Function to return beam offsets (as would be used in a linmos
    # parset) for the full set of beams for a given field.
    #  Required inputs:
    #     * NOW - date/time of current pipeline run
    #     * FIELD - name of field
    #     * BEAM_MAX - how many beams to consider
    #  Returns: $LINMOS_BEAM_OFFSETS (in the process, setting $footprintOut)

    setFootprintFile
    LINMOS_BEAM_OFFSETS=`grep -A$[BEAM_MAX+1] Beam ${footprintOut} | tail -n $[BEAM_MAX+1] | sed -e 's/(//g' | sed -e 's/)//g' | awk '{printf "linmos.feeds.beam%02d = [%6.3f, %6.3f]\n",$1,-$4,$5}'`
}

function getBeamCentre()
{
    # Function to return the centre direction of a given beam
    #  Required inputs:
    #     * NOW - date/time of current pipeline run
    #     * FIELD - name of field
    #     * BEAM - the beam ID to obtain the centre for
    #     * BEAM_MAX - how many beams to consider
    #  Returns: $DIRECTION (in the process, setting $footprintOut, $ra, $dec)

    setFootprintFile
    awkTest="\$1==$BEAM"
    ra=`grep -A$[BEAM_MAX+1] Beam ${footprintOut} | tail -n $[BEAM_MAX+1] | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g' | awk $awkTest | awk '{print $6}'`
    ra=`echo $ra | awk -F':' '{printf "%sh%sm%s",$1,$2,$3}'` 
    dec=`grep -A$[BEAM_MAX+1] Beam ${footprintOut} | tail -n $[BEAM_MAX+1] | sed -e 's/,/ /g' | sed -e 's/(//g' | sed -e 's/)//g' | awk $awkTest | awk '{print $7}'`
    dec=`echo $dec | awk -F':' '{printf "%s.%s.%s",$1,$2,$3}'` 
    DIRECTION="[$ra, $dec, J2000]"
}


##############################
# JOB STATISTIC MANAGEMENT

# Need to declare the stats directory variable here, so it is seen
# from within the various slurm jobs
if [ "${BASEDIR}" == "" ]; then
    stats=stats
else
    stats=${BASEDIR}/stats
fi
mkdir -p $stats

function writeStats()
{
    # usage: writeStats ID DESC RESULT NCORES REAL USER SYS VM RSS format
    #   where format is either txt or csv. Anything else defaults to txt
    format=${10}
    if [ $# -ge 9 ] && [ "$format" == "csv" ]; then
	echo $@ | awk '{printf "%s,%s,%s,%s,%s,%s,%s,%s,%s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9}'
    else
	echo $@ | awk '{printf "%10s%10s%40s%9s%10s%10s%10s%10s%10s\n",$1,$2,$3,$4,$5,$6,$7,$8,$9}'
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
    writeStats "JobID" "nCores" "Description" "Result" "Real" "User" "System" "PeakVM" "PeakRSS" $format
}

function extractStats()
{
    # usage: extractStats logfile nCores ID ResultCode Description [format]
    # format is optional. If not provided, output is written to stdout
    #   if provided, it is assumed to be a list of suffixes - these can be either txt or csv.
    #      If txt - output is written to $stats/stats-ID-DESCRIPTION.txt as space-separated ascii
    #      If csv - output is written to $stats/stats-ID-DESCRIPTION.csv as comma-separated values
    #
    # Must also have defined the variable NUM_CPUS. If not defined, it will be set to 1
    # (this is used for the findWorkerStats() function)

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
    
    parseLog $STATS_LOGFILE    

    if [ $# -lt 6 ]; then
	formatlist="stdout"
    else
	formatlist=$6
    fi
    
    for format in `echo $formatlist | sed -e 's/,/ /g'`; do

	if [ $format == "txt" ]; then
	    output=${stats}/stats-${STATS_ID}-${STATS_DESC}.txt
	elif [ $format == "csv" ]; then
	    output=${stats}/stats-${STATS_ID}-${STATS_DESC}.csv
	else
	    output=/dev/stdout
	fi

	writeStatsHeader $format > $output
	if [ `grep "(1, " $1 | wc -l` -gt 0 ]; then
	    writeStats $STATS_ID $NUM_CORES "${STATS_DESC}_master"     $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_MASTER  $PEAK_RSS_MASTER  $format >> $output
	    writeStats $STATS_ID $NUM_CORES "${STATS_DESC}_workerPeak" $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_WORKERS $PEAK_RSS_WORKERS $format >> $output
	    writeStats $STATS_ID $NUM_CORES "${STATS_DESC}_workerAve"  $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $AVE_VM_WORKERS  $AVE_RSS_WORKERS  $format >> $output
	else                                                                      
	    writeStats $STATS_ID $NUM_CORES $STATS_DESC                $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_MASTER  $PEAK_RSS_MASTER  $format >> $output
	fi

    done

}

function parseLog()
{

    TIME_JOB_REAL="---"
    TIME_JOB_SYS="---"
    TIME_JOB_USER="---"
    PEAK_VM_MASTER="---"
    PEAK_RSS_MASTER="---"

    if [ ${NUM_CORES} -ge 2 ] && [ `grep "(1, " $1 | wc -l` -gt 0 ]; then
        # if here, job was a distributed job
        if [ `grep "(0, " $1 | grep "Total times" | wc -l` -gt 0 ]; then
            TIME_JOB_REAL=`grep "(0, " $1 | grep "Total times" | tail -1 | awk '{print $16}'`
            TIME_JOB_SYS=`grep "(0, " $1  | grep "Total times" | tail -1 | awk '{print $14}'`
            TIME_JOB_USER=`grep "(0, " $1 | grep "Total times" | tail -1 | awk '{print $12}'`
        fi
        if [ `grep "(0, " $1 | grep "PeakVM" | wc -l` -gt 0 ]; then
            PEAK_VM_MASTER=`grep "(0, " $1 | grep "PeakVM" | tail -1 | awk '{print $12}'`
            PEAK_RSS_MASTER=`grep "(0, " $1 | grep "PeakVM" | tail -1 | awk '{print $15}'`
        fi
	findWorkerStats $1
    else
        # if here, it was a serial job
        if [ `tail $1 | grep "Total times" | wc -l` -gt 0 ]; then
            TIME_JOB_REAL=`tail $1 | grep "Total times" | awk '{print $16}'`
            TIME_JOB_SYS=`tail $1 | grep "Total times" | awk '{print $14}'`
            TIME_JOB_USER=`tail $1 | grep "Total times" | awk '{print $12}'`
        fi
        if [ `tail $1 | grep "PeakVM" | wc -l` -gt 0 ]; then
            PEAK_VM_MASTER=`tail $1 | grep "PeakVM" | awk '{print $12}'`
            PEAK_RSS_MASTER=`tail $1 | grep "PeakVM" | awk '{print $15}'`
        fi
    fi

}

function findWorkerStats()
{
    logfile=$1
    tmpfile=tmpout
    
    PEAK_VM_WORKERS="---"
    PEAK_RSS_WORKERS="---"
    AVE_VM_WORKERS="---"
    AVE_RSS_WORKERS="---"

    grep "PeakVM" $logfile | grep -v "(0, " > $tmpfile

    if [ `wc -l $tmpfile | awk '{print $1}'` -gt 0 ]; then

        awkfile=stats.awk
        cat > $awkfile <<EOF
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

        tmpfile2="$tmpfile.2"
        rm -f $tmpfile2
        if [ "${NUM_CORES}" == "" ]; then
            NUM_CORES=2
        fi
        for((i=1;i<${NUM_CORES};i++)); do

	    grep "($i, " $tmpfile | tail -1 >> $tmpfile2
	    
        done
        
        results=`awk -f $awkfile $tmpfile2`
        PEAK_VM_WORKERS=`echo $results | awk '{print $3}'`
        PEAK_RSS_WORKERS=`echo $results | awk '{print $6}'`
        AVE_VM_WORKERS=`echo $results | awk '{print $2}'`
        AVE_RSS_WORKERS=`echo $results | awk '{print $5}'`

    fi
    
    rm -f $tmpfile $tmpfile2 $awkfile


}
