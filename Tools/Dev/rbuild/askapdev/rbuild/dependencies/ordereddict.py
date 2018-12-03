## @file
#  Module to gather dependency information for ASKAP packages
#
# @copyright (c) 2006 CSIRO
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
#  @author Malte Marquarding <malte.marquarding@csiro.au>
#
from collections import OrderedDict as OD

## This is an ordered dict, i.e. the keys are in the order in
#  which they have been added. It has the same interface as dict
#  but only the functions which are in use have been implemented
class OrderedDict(OD):
    ## Create an empty container
    #  @param self the object reference
    def __init__(self, items=None):
        if items is None:
            items = {}
        super(OrderedDict, self).__init__(items)

    ## Move an existing item to the end of the container
    #  @param self the object reference
    #  @param key the key of the item
    def toend(self, key):
        value = self.pop(key)
        self[key] = value
