from askapdev.rbuild.builders import Setuptools as Builder

import glob
import os
import shutil

from askapdev.rbuild.builders import Builder

class myBuilder(Builder):
    def __init__(self):
        Builder.__init__(self, pkgname='.', archivename=None, extractdir=None)

    def _build(self):
        pass

    def _install(self):
        files = glob.glob("*.py")
        ASKAP_ROOT = os.environ['ASKAP_ROOT']
        SCONS_TOOLS_DIR = os.path.join(ASKAP_ROOT, 'share', 'scons_tools')

        if not os.path.exists(SCONS_TOOLS_DIR):
            os.makedirs(SCONS_TOOLS_DIR)

        for file in glob.glob('*.py'):
            if file != 'build.py':
                shutil.copy(file, SCONS_TOOLS_DIR)


builder = myBuilder()
builder.remote_archive = "scons-3.0.1.tar.gz"
builder.nowarnings = True
builder.build()
