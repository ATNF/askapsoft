## Package for various utility functions to execute build and shell commands
#
# @copyright (c) 2011 CSIRO
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

from runcmd import runcmd


def get_svn_branch_info():
    '''Return the branch of the repository we are using.
    e.g. ['trunk'], ['releases', '0.3'], ['features', 'TOS', 'JC'] etc
    '''
    ASKAP_ROOT = os.environ['ASKAP_ROOT']
    # In future layout will find Code in src subdirectory.
    SRC_DIR = os.sep.join((ASKAP_ROOT, 'src'))
    if os.path.isdir(SRC_DIR):
        CODE_DIR = os.sep.join((SRC_DIR, 'Code'))
    else: # current (old) layout
        CODE_DIR = os.sep.join((ASKAP_ROOT, 'Code'))

    bi = ['Unknown']    # Define here to handle svn runtime failures or
                        # svn output format changes.
    try:
        for line in runcmd('svn info %s' % CODE_DIR)[0].split('\n'):
            if line.startswith('URL:'):
                repo = line.split(os.sep)[3]
                # handle differences in layout of SDP and TOS.
                if repo == "askapsoft":
                    topdir = 'Src'
                elif repo == "askapsdp":
                    topdir = 'askapsdp'
                else:
                    # just spit on the toplevel directory and down.
                    # https://foo.com/topdir
                    groups = line.split(os.sep)
                    topdir = os.sep.join(groups[:4])

                bi = line.split(topdir)[1].split(os.sep)[1:-1]
                break
    except:
        pass # bi already defined.

    return bi


if __name__ == '__main__':
    print get_svn_branch_info()
