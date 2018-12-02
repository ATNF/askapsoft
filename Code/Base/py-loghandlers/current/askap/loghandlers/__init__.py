"""
==================================================================
Module :mod:`askap.loghandler` -- Logging handlers custom to ASKAP
==================================================================

This module provides the following handlers:
    * :class:`IceHandler` - logging over ZeroC IceStorm

"""
__all__ = ['IceHandler']

from askap.loghandlers.icehandler import IceHandler
