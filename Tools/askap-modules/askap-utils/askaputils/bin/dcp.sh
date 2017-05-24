#!/bin/bash
#
# Script call the slurm batch script /<ASKAP MODULES>/askaputils/slurm/dcp.sbatch to schedule a job to
# do a copy operation.
#
# This can be called using the dcp alias that has been setup in the askaputils module
#
# ----
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
# @author Eric Bastholm <eric.bastholm@csiro.au>
#

copy() {
    DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    sbatch ${DIR}/../slurm/dcp.sbatch "$@"
}

# Process the command line parameters.
#
# Valid parameters:
#   -h | ?        Help
#
processCmdLine() {

  # For option processing
  OPTIND=1

  # Process our options
  while getopts "h?" opt "$@"; do
      case "$opt" in

      h|\?)
          printf "dcp: Copy from source to destination using dcp from the mpifileutils module.\n"
          printf "dcp usage: dcp [source] [destination]\n"
          exit $NO_ERROR
          ;;
      esac
  done

  shift $((OPTIND-1))

  [ "$1" = "--" ] && shift

}

processCmdLine "$@"

copy "$@"