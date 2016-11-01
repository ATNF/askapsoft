#!/bin/bash -l
#
# A script to set up all necessary subdirectories required for the
# pipeline processing.
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

####################
# Define & create directories

parsets=${BASEDIR}/parsets
logs=${BASEDIR}/logs
slurms=${BASEDIR}/slurmFiles
slurmOut=${BASEDIR}/slurmOutput
tools=${BASEDIR}/tools
metadata=${BASEDIR}/metadata
stats=${BASEDIR}/stats

mkdir -p $parsets
lfs setstripe -c 1 $parsets
mkdir -p $logs
lfs setstripe -c 1 $logs
mkdir -p $slurms
lfs setstripe -c 1 $slurms
mkdir -p $slurmOut
lfs setstripe -c 1 $slurmOut
mkdir -p $tools
lfs setstripe -c 1 $tools
mkdir -p $metadata
lfs setstripe -c 1 $metadata
mkdir -p $stats
lfs setstripe -c 1 $stats


# temp directory for certain files that we don't need to keep
tmp=/tmp/tmp`whoami`pipe
mkdir -p $tmp
