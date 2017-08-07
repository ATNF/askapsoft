# Copyright (c) 2014 CSIRO
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
Monitoring
----------

Implementation of Ice interfaces to provide monitoring information

.. literalinclude:: ../../../../Interfaces/slice/current/MonitoringProvider.ice

"""
from dogpile.core import ReadWriteMutex
from askap.time import bat_now
from askap.slice import MonitoringProvider
from askap.interfaces.monitoring import (MonitorPoint, MonitoringProvider,
                                         PointStatus)
from .typedvalues import TypedValueMapper

__all__ = ["MONITOR", "MonitoringProviderImpl"]


class PointMapper(object):
    def __init__(self):
        """
        :class:`TypedValueMapper` encoded Point representation
        """
        self._mapper = TypedValueMapper()

    def __call__(self, name, value, timestamp, status="OK", unit=""):
        """Get 'encoded' monitoring point"""
        typedmap = self._mapper({'value': value})
        items = {'name': name, 'timestamp': timestamp,
                 'unit': unit,
                 'status': getattr(PointStatus, status.upper()),
                 'value': typedmap['value']}
        return MonitorPoint(**items)


class MonitoringBuffer(object):
    def __init__(self):
        """Thread safe buffer of monitoring points"""
        self._mutex = ReadWriteMutex()
        self._buffer = {}

    def add_points(self, points, timetag=None):
        """Add a :class:`dict` of point(s) to the buffer. This is to support points
        without metadata (old-style monitoring points)"""
        try:
            timestamp = timetag or bat_now()
            self._mutex.acquire_write_lock()
            for k, v in points.items():
                self._buffer[k] = {'value': v, 'name': k,
                                   'timestamp': timestamp}
        finally:
            self._mutex.release_write_lock()

    def add_full_points(self, points):
        """ points is a list of dict, each is a monitor point """
        try:
            self._mutex.acquire_write_lock()
            for point in points:
                name = point['name']
                mon_point = {'name': name, 
		             'timestamp': point['timestamp'] or bat_now(),
                     'unit': point['unit'] or '',
                     'status': point['status'] or 'OK',
                     'value': point['value']}
                self._buffer[name] = mon_point
        finally:
            self._mutex.release_write_lock()

    def add(self, name, value, timestamp=None, status="OK", unit=""):
        """Add a monitor point with metadata"""
        try:
            timestamp = timestamp or bat_now()
            point = {'name': name, 'timestamp': timestamp,
                     'unit': unit,
                     'status': status,
                     'value': value}
            self._mutex.acquire_write_lock()
            self._buffer[name] = point
        finally:
            self._mutex.release_write_lock()

    def remove(self, keys):
        points = keys
        if isinstance(keys, basestring):
            points = (keys,)
        try:
            self._mutex.acquire_write_lock()
            for k in points:
                if k in self._buffer:
                    self._buffer.pop(k)
        finally:
            self._mutex.release_write_lock()

    def clear(self):
        """
        Clear the buffer
        """
        try:
            self._mutex.acquire_write_lock()
            self._buffer.clear()
        finally:
            self._mutex.release_write_lock()

    def get(self, keys):
        """Get given point names `keys` from buffer

        :param list keys: a list of point names.

        """
        if isinstance(keys, basestring):
            keys = (keys,)
        try:
            self._mutex.acquire_read_lock()
            return [self._buffer[k] for k in keys if k in self._buffer]
        finally:
            self._mutex.release_read_lock()


MONITOR = MonitoringBuffer()
"Global :obj:`MonitoringBuffer` (singleton)"


# noinspection PyUnusedLocal
class MonitoringProviderImpl(MonitoringProvider):
    """
    The implementation of the ice interface to provide monitoring points.
    """
    def __init__(self):

        self._point_mapper = PointMapper()

    # noinspection PyIncorrectDocstring
    def get(self, pointnames, current=None):
        """
        Get a list of monitoring points from the global buffer :attr:`MONITOR`

        :param pointnames: names of the points to retrieve
        :return: list of :class:`MonitorPoint`
        """

        points = []
        try:
            points = MONITOR.get(pointnames)
        except:
            pass
        return [self._point_mapper(**p) for p in points]
