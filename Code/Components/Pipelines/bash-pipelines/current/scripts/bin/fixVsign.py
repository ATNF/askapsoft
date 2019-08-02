#!/usr/bin/env python
#
# fixVsign.py
#
# A python script to multiply the pixel values of an image by -1 -
# primarily aimed at Stokes V images, getting them consistent with the
# IAU/IEEE definition of Stokes V.
#
# @copyright (c) 2019 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the License,
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
import casacore.images.image as im
import numpy as np

historyComment="Pixels multiplied by -1, to conform to IAU/IEE Stokes-V definition"

#############
if __name__ == '__main__':

    parser = argparse.ArgumentParser(description="A simple tool to invert the sign of stokes-V images")
    parser.add_argument("-i","--image", dest="image")

    args = parser.parse_args()

    if args.image == '':
        print("Image not given - you need to use the -i option")
        exit(0)

    if args.image[-5:] == ".fits":

        hdulist = fits.open(args.image,'update')
        hdr1 = hdulist[0].header

        hdulist[0].data *= -1.
        
        hdr1.add_history(historyComment)

        hdulist.flush()

    else:

        img = im(args.image)
        arr = img.getdata()
        arr = arr * -1.
        img.putdata(arr)
        # No way of writing a history comment, as far as I can tell
