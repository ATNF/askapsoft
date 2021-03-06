# @file get_svn_revision.py
# Fetch the subversion revision number from the repository
#
# @copyright (c) 2006,2014 CSIRO
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
# @author Robert Crida <robert.crida@ska.ac.za>
#

from runcmd import runcmd
from get_vcs_type import is_git
import os

def get_svn_revision():
    try:
        (stdout, stderr, returncode) = runcmd('svnversion', shell=True)
        if returncode == 0 and stdout and stdout[0].isdigit():
            return stdout.rstrip()
        else:
            if is_git():
                return get_git_revision()
            return "unknown"
    except:
        return "unknown"

def get_git_revision():
    try:
        (stdout, stderr, returncode) = runcmd('git describe --tags --always', shell=True)
        if returncode == 0:
            return stdout.rstrip()
        else:
            return "unknown"
    except:
        return "unknown"
