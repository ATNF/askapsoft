## Package for various utility functions to execute build and shell commands
#
# @copyright (c) 2013 CSIRO
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
# @author Tony Maher <Tony.Maher@csiro.au>
#
import os

DEV_DIR = os.path.join("Tools", "Dev")
ASKAP_ROOT = os.environ['ASKAP_ROOT']

def in_dev_tree():
    '''Are we inside the "Tools/Dev" tree (ASKAPsoft developer tools)'''
    rpath = os.path.relpath(os.getcwd(), ASKAP_ROOT)
    return DEV_DIR in rpath

if __name__ == "__main__":
    print in_dev_tree() and "Yes" or "No"
