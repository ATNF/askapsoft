#!/usr/bin/env python
#
# A python script to update the header keywords of FITS files. The
# headers able to be updated are a small defined set: PROJECT, SBID,
# DATE-OBS, DURATION.
#
# It also allows the specification of HISTORY statments, by giving a
# list of strings following the arguments for the above
#
# Example Usage:
#   updateFITSheaders.py --project=AS034 --SBID=1234 --DATE-OBS="2017-01-20T12:34:45" --DURATION=12345.6 "Made by me" "Not by you"
#
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

import argparse
import astropy.io.fits as fits

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('--fitsfile', type=str, help='FITS file to update')
parser.add_argument('--telescope', type=str, default="", help='TELESCOP keyword')
parser.add_argument('--project', type=str, default="", help='OPAL project ID for this observation')
parser.add_argument('--sbid', type=str, default="", help='Scheduling block ID for this observation')
parser.add_argument('--dateobs', type=str, default="", help='DATE-OBS string (YYYY-MM-DDTHH:MM:SS) for this observation')
parser.add_argument('--duration', type=str, default="", help='Length of this observation in sec')
parser.add_argument('history', metavar='hist', type=str, default="", nargs='+', help='A HISTORY statement')
options = parser.parse_args()

if options.fitsfile == "":
    exit(1)

hdulist = fits.open(options.fitsfile,'update')
hdr1 = hdulist[0].header

if options.telescope != '':
    hdr1['TELESCOP'] = options.telescope
if options.project != '':
    hdr1['PROJECT'] = options.project
if options.sbid != '':
    hdr1['SBID'] = options.sbid
if options.dateobs != '':
    hdr1['DATE-OBS'] = options.dateobs
if options.duration != '':
    hdr1['DURATION'] = options.duration

for history in options.history:
    hdr1.add_history(history)

hdulist.flush()



