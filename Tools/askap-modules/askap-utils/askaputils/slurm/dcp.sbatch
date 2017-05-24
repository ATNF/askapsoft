#!/bin/bash
#
# Script to copy a source to a destination using the dcp utility.
# This submits a job to the copy queue and uses dcp to efficiently perform the copy.
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

#SBATCH --export=NONE
#SBATCH --output=/group/askap/logs/dcp2-copy-logs/%j-copy.out
#SBATCH --error=/group/askap/logs/dcp2-copy-logs/%j-copy.err
#SBATCH --account=askap
#SBATCH --clusters=zeus
#
#SBATCH --partition=copyq
#SBATCH --time=7:59:00
 
module load mpifileutils
mpirun -np 12 dcp -p -d info $1 $2

