#!/usr/bin/env bash
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
    cp $1 $slurmOut/${filename}__${NOW}.${extension}
    
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
# JOB STATISTIC MANAGEMENT

# Need to declare the stats directory variable here, so it is seen
# from within the various slurm jobs
stats=stats
mkdir -p $stats

function writeStats()
{
    # usage: writeStats ID DESC RESULT REAL USER SYS VM RSS format
    #   where format is either txt or csv. Anything else defaults to txt
    if [ $# -ge 7 ]; then
	if [ $# -ge 8 ] && [ "$9" == "csv" ]; then
	    echo $@ | awk '{printf "%s,%s,%s,%s,%s,%s,%s,%s\n",$1,$2,$3,$4,$5,$6,$7,$8}'
	else
	    echo $@ | awk '{printf "%10s%40s%9s%10s%10s%10s%10s%10s\n",$1,$2,$3,$4,$5,$6,$7,$8}'
	fi
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
    writeStats "JobID" "Description" "Result" "Real" "User" "System" "PeakVM" "PeakRSS" $format
}

function extractStats()
{
    # usage: extractStats logfile ID ResultCode Description [format]
    # format is optional. If not provided, output is written to stdout
    #   if provided, it is assumed to be a list of suffixes - these can be either txt or csv.
    #      If txt - output is written to $stats/stats-ID-DESCRIPTION.txt as space-separated ascii
    #      If csv - output is written to $stats/stats-ID-DESCRIPTION.csv as comma-separated values
    #
    # Must also have defined the variable NUM_CPUS. If not defined, it will be set to 1
    # (this is used for the findWorkerStats() function)

    STATS_LOGFILE=$1
    STATS_ID=$2
    RESULT=$3
    STATS_DESC=$4

    if [ "$RESULT" == "0" ]; then
        RESULT_TXT="OK"
    else
        RESULT_TXT="FAIL"
    fi
    
    parseLog $STATS_LOGFILE    

    if [ $# -lt 5 ]; then
	formatlist="stdout"
    else
	formatlist=$5
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
	    writeStats $STATS_ID "${STATS_DESC}_master" $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_MASTER $PEAK_RSS_MASTER $format >> $output
	    writeStats $STATS_ID "${STATS_DESC}_workerPeak" $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_WORKERS $PEAK_RSS_WORKERS $format >> $output
	    writeStats $STATS_ID "${STATS_DESC}_workerAve" $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $AVE_VM_WORKERS $AVE_RSS_WORKERS $format >> $output
	else
	    writeStats $STATS_ID $STATS_DESC $RESULT_TXT $TIME_JOB_REAL $TIME_JOB_USER $TIME_JOB_SYS $PEAK_VM_MASTER $PEAK_RSS_MASTER $format >> $output
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

    if [ `grep "(1, " $1 | wc -l` -gt 0 ]; then
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
        if [ "${NUM_CPUS}" == "" ]; then
            NUM_CPUS=2
        fi
        for((i=1;i<${NUM_CPUS};i++)); do

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
