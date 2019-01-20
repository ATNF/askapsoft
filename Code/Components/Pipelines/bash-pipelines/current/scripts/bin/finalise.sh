#!/bin/bash -l
#
# This script takes care of tasks that need to be run either upon
# completion of all jobs from a single processASKAP.sh call, or can be
# run once all jobs have been submitted to the queue (and hence their
# job IDs are known). 
# Included here are:
#  * Creation of a user script to indicate progress of queued jobs.
#  * Creation of a user script to kill all launched jobs
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

##############################

if [ "${SUBMIT_JOBS}" == "true" ] && [ "${ALL_JOB_IDS}" != "" ]; then

    # Create tool to report progress

    reportscript="${tools}/reportProgress-${NOW}.sh"
    cat > "$reportscript" <<EOF
#!/bin/bash -l
#
# A script to report progress on all jobs associated with a given call of processASKAP.sh.
# It will run squeue just on the appropriate job IDs, showing those that have not yet completed.
# Providing the '-v' option will also show a list of jobs with detailed descriptions.
#

verbose="false"
while getopts ':v' opt
do
    case \$opt in
	v) verbose="true";;
	\?) echo "ERROR: Invalid option"
	    echo "   Usage: \$0 [-v]"
	    echo "     -v : verbose mode, shows list of jobs with descriptions"
	    exit 1;;
    esac
done

if [ "\$verbose" == "true" ]; then

    echo "SUMMARY OF JOBS:"
    cat "$JOBLIST"
    echo " "

fi

squeue --jobs="${ALL_JOB_IDS},${ID_STATS}"

EOF
    # make it executable
    chmod a+x "$reportscript"

    ##############################

    # Create tool to kill all jobs with this datestamp
    killscript="${tools}/killAll-${NOW}.sh"
    joblist=$(echo "${ALL_JOB_IDS}" | sed -e 's/,/ /g')
    cat > "$killscript" <<EOF
#!/bin/bash -l
#
# A simple script to run scancel on all jobs associated with a 
# given call of processASKAP.sh
#

echo "This will run scancel on all remaining jobs (run 'reportProgress.sh' to see them)."
read -p "Are you sure? (type yes to continue) : " ANS

if [ "\$ANS" == "yes" ]; then

    scancel ${joblist}

else

    echo "OK, nothing cancelled"

fi
EOF
    # make it executable
    chmod a+x "$killscript"

    ##############################

    # Make symbolic links that don't have the timestamp
    function makeLink()
    {
        # first, remove the timestamp
        sedstr="s/-${NOW}//g"
        link=$(echo "$1" | sed -e "$sedstr")
        # remove trailing suffix
        link=${link%%.*}
        # remove all leading paths
        link=${link##*/}
        # make the link, and clobber any existing link
        ln -s -f "$1" "$link"
    }

    # make links to the current scripts
    makeLink "$reportscript" 
    makeLink "$killscript"

    # make links to the current job list, both at the top level *and* in each output directory
    makeLink "$JOBLIST"
    IFS="${IFS_FIELDS}"
    for FIELD in ${FIELD_LIST}; do
        CWD=$(pwd)
        if [ -e "${ORIGINAL_OUTPUT}/${FIELD}" ]; then
            cd "${ORIGINAL_OUTPUT}/${FIELD}"
            makeLink "$JOBLIST"
            cd "${CWD}"
        fi
    done
    IFS="${IFS_DEFAULT}"

fi
