# Non standard install.
# In scons, tools must live in a directory and be specified by a file system
# path.  They cannot be in a python package.
# We want to install into the top level ASKAPsoft tree (like all other tools)
# and not hard coded to a location in the Tools subdirectory.
#
# Daniel Collins, 15 July 2016: copying this from the junit build.py.
# Whatever JUnit does is exactly what mockito will require, as they are both
# Java libraries required only for unit tests.
# Mockito is the current state of the art for Java mocking.

import glob
import shutil
import os

from askapdev.rbuild.builders import Builder

class myBuilder(Builder):
    def __init__(self):
        Builder.__init__(self, pkgname='.', archivename=None, extractdir=None)

    def _build(self):
        pass

    def _install(self):
        ASKAP_ROOT = os.environ['ASKAP_ROOT']
        JAR_DIR = os.path.join(ASKAP_ROOT, 'lib')

        if not os.path.exists(JAR_DIR):
            os.makedirs(JAR_DIR)

        for file in glob.glob('*.jar'):
            shutil.copy(file, JAR_DIR)

builder = myBuilder()

archives = [
    "mockito-core-2.0.9-beta-javadoc.jar",
    "mockito-core-2.0.9-beta-sources.jar",
    "mockito-core-2.0.9-beta.jar",
]

for a in archives:
    builder.remote_archive = a
    builder.build()
