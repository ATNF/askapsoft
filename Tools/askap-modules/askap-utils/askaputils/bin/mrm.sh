#!/bin/bash
#
# Script to remove a list of files and-or directories (recursively) using munlink
# rather than the usual rm so that it is more lustre friendly.
#
# This can be called using the mrm alias that has been setup in the askaputils module
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

mrm() {
  for var in "$@"
  do

    # file args
    if [ -f "$var" ]; then
      munlink "$var"
    fi

    # symbolic link args
    if [ -L "$var" ]; then
      munlink "$var"
    fi

    # directory args
    if [ -d "$var" ]; then
      find "$var" -type f -print0 | xargs -0 -r munlink # files
      find "$var" -type l -print0 | xargs -0 -r munlink # symbolic links
      find "$var" -depth -type d -empty -delete      # directories
      rm -rf "$var"
    fi
  done
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
          printf "mrm: delete files or directories (recursively) with munlink.\n"
          printf "mrm usage: mrm [file... | directory...]\n"
          exit $NO_ERROR
          ;;
      esac
  done

  shift $((OPTIND-1))

  [ "$1" = "--" ] && shift

}

processCmdLine "$@"

mrm "$@"
