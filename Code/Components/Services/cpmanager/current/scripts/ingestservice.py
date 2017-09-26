#!/usr/bin/env python

import sys, Ice
import time, os

from askap.ingest.server import IngestManager

# noinspection PyUnresolvedReferences
from askap import logging


def main():
    logging.init_logging(sys.argv)
    logger = logging.getLogger(__file__)
    communicator = Ice.initialize(sys.argv)

    try:
        ingest_manager = IngestManager(communicator)
        ingest_manager.run()
    except Exception as ex:
        logger.error(str(ex))
        print >>sys.stderr, ex
        sys.exit(1)
    finally:
        communicator.destroy()

if __name__ == "__main__":
    main()
