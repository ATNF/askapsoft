# Copyright (c) 2017 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
"""
===============================
Module :mod:`askap.coordinates`
===============================

Helper functions to deal with RA/DEC from strings in sexagsimal

:author: Malte Marquarding <Malte.Marquarding@csiro.au>
"""

__all__ = ['parse_direction', 'encode_direction']


def deg2ra(value):
    """
    encode given `value` as sexagesimal string

    :param float value:  RA in decimal degrees
    :return str: sring in sexagesmial form

    """
    value = round(value, 5)
    value /= 15.0
    hh, r1 = divmod(value, 1)
    mm, r2 = divmod(r1*60, 1)
    ss = r2*60
    return "{0:02d}:{1:02d}:{2:06.3f}".format(int(hh), int(mm), ss)


def dec2deg(value):
    p = [float(v) for v in value.split(":")]
    sign = 1.0
    if p[0] < 0.0:
        sign = -1.0
        p[0] = abs(p[0])
    return (((p[2]/60+p[1])/60)+p[0])*sign


def ra2dec(value):
    return dec2deg(value)*15.0


def deg2dec(value):
    """
    encode given `value` as sexagesimal string

    :param float value:  declination in decimal degrees
    :return str: sring in sexagesmial form

    """
    value = round(value, 5)
    sign = 1
    if value < 0:
        sign = -1
        value = abs(value)
    dd, r1 = divmod(value, 1)
    mm, r2 = divmod(r1*60, 1)
    ss = r2*60
    return "{0:+03d}:{1:02d}:{2:05.2f}".format(int(sign*dd), int(mm), ss)


def encode_direction(ra, dec):
    """
    Encode RA/Dec pair as sexagesimal string

    :param float ra: the RA in degrees
    :param float dec: the Dec in degress
    :return str: sexagesimal comma separate encoding

    """
    return ",".join((deg2ra(ra), deg2dec(dec)))


def parse_direction(value):
    """
    Take a `value` of ra,dec and return as a tuple in decimal degrees

    :param value: sexagesimal or decimal degree string "ra,dec", e.g.
                  "00:29:51.977,-44:25:27.12" or "180.0,-64.34"

    :return: tuple of (ra, dec) in decimal degress

    """
    if value.count(":") == 0:
        return [float(i) for i in value.split(",")]
    ra, dec = value.split(",")
    if dec.count(":") == 0:
        dec = dec.replace(".", ":", 2)
    return ra2dec(ra), dec2deg(dec)
