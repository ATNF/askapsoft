# Copyright (c) 2015,2016 CSIRO
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
========================
Module :mod:`askap.timer`
========================

A utility class for measuring elaspsed time. Good for basic timing and
profiling.

A more portable approach would be to use the timeit module from
the standard library.
"""
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
=========================
Module :mod:`askap.timer`
=========================

Timer utitlities
"""
import time


class Timer:
    """Simple benchmark timing class """

    def __init__(self):
        self.starttime = None
        self.endtime = None

    def start(self):
        """
        Start the timer
        :return:
        """
        self.starttime = time.time()

    def stop(self):
        """
        Stop the timer
        :return:
        """
        self.endtime = time.time()

    def sec(self):
        """
        Return elepased time
        :return: time in seconds ot `None` if not run
        """
        if self.starttime is None or self.endtime is None:
            return None
        return self.endtime - self.starttime

    def msec(self):
        """
        Return elepased time
        :return: time in milliseconds ot `None` if not run
        """
        sec = self.sec()
        if sec is None:
            return None
        else:
            return sec * 1000

    def elapsedmsec(self, msg):
        """Return as string message"""
        msec = self.msec()
        if msec is None:
            print 'Elapsed time for "{0}" undefined'.format(msg)
        else:
            print '{0} took {1:.1f} msec'.format(msg, msec)

    def elapsed(self, msg):
        """Return as string message"""
        sec = self.sec()
        if sec is None:
            print 'Elapsed time for "{0}" undefined'.format(msg)
        else:
            print '{0} took {1:.1f} sec'.format(msg, sec)
