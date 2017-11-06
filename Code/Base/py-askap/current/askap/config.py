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
==========================
Module :mod:`askap.config`
==========================

Configuration utilities

:author: Malte Marquarding <Malte.Marquarding@csiro.au>
"""
import os

__all__ = ['get_config']


def get_config(module_name, file_name):
    """Return package relative configuration directory. This could for example
    contain ice configuration and slice files.

    :param module_name: the name of the python module
    :param file_name: the name of the resource file

    """
    from pkg_resources import resource_filename
    cfg = resource_filename(module_name,
                            os.path.join("config", file_name))
    return cfg
