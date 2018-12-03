from __future__ import print_function
## Package for various utility functions to execute build and shell commands
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

ASKAP_ROOT = os.environ["ASKAP_ROOT"]

def get_dep_installpath(varname, dirname='.'):
    '''
    Return the full system path to the dependency package reference by the
    given varname.
    The format for dependency files is:
    <varname>=<package path>[;libraries]
    The path to the package is relative to RBS_SRC.
    '''
    fn = os.path.join(dirname, 'dependencies.default')
    with open(fn) as fh:
        all_lines = (line.strip() for line in fh) # All lines inc the blank ones
        all_lines = (line for line in all_lines if line) # Non-blank lines
        all_lines = (line for line in all_lines if not line.startswith('#')) # ignore comments

        for line in all_lines:
            vn, pkgpath = line.split('=')
            if vn == varname:
                if ';' in line:
                    pkgpath, libs = pkgpath.split(';')
                return os.path.join(ASKAP_ROOT, pkgpath, 'install')


if __name__ == '__main__':
    path = os.path.join(ASKAP_ROOT, '3rdParty', 'Ice', 'Ice-3.6.3')
    print(get_dep_path(path,'db'))
