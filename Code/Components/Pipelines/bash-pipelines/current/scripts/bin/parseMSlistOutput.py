#!/usr/bin/env python
#
# A python script to parse the output from mslist and return one of a
# defined set of values: RA, Dec, Epoch or Frequency (the latter is
# the central frequency).
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

parser = argparse.ArgumentParser(formatter_class=argparse.ArgumentDefaultsHelpFormatter)
parser.add_argument('--file', type=str, help='MSlist output file to parse')
parser.add_argument('--val', type=str, help='Value to extract: RA, Dec, Epoch, Freq, nChan, Bandwidth, chan0')
parser.add_argument('--units', type=bool, default=False, help='Print units (true), or convert to Hz (false)?')
options = parser.parse_args()

if options.file == "":
    exit(1)

doUnits=options.units
hasUnits=False
if options.val == "RA":
    str='RA'
    nextStr='Decl'
elif options.val == "Dec":
    # Need a space before Decl so that we can catch minus signs
    str=' Decl'
    nextStr='Epoch'
elif options.val == "Epoch":
    str='Epoch'
    nextStr='nRows'
elif options.val == "Freq":
    str='CtrFreq'
    nextStr='Corrs'
    hasUnits=True
elif options.val == "nChan":
    str='#Chans'
    nextStr='Frame'
elif options.val == 'Bandwidth':
    str='TotBW'
    nextStr='CtrFreq'
    hasUnits=True
elif options.val == "chan0":
    str='Ch0'
    nextStr='ChanWid'
    hasUnits=True
elif options.val == "chanw":
    str='ChanWid'
    nextStr='TotBW'
    hasUnits=True
else:
    print('Bad value type')
    exit(1)

fin=open(options.file)
locBeg=-1
locEnd=-1
val=''
for line in fin:
    if (locBeg < 0):
        locBeg = line.find(str)
        locEnd = line.find(nextStr)
        if (locBeg > 0) and hasUnits:
            units=line[locBeg:locEnd].replace('(',' ').replace(')',' ').split()[1]
    elif val=='':
        val=line[locBeg:locEnd].split()[0].strip()
        if hasUnits:
            if doUnits:
                val='%s%s'%(val,units)
            else:
                factor={'Hz':1.,'kHz':1.e3,'MHz':1.e6,'GHz':1.e9,'THz':1.e12}
                val = float(val) * factor[units]
            

fin.close()

print(val)

