import os
from askapdev.rbuild.builders import Autotools as Builder
import askapdev.rbuild.utils as utils

builder = Builder()
builder.remote_archive = "xerces-c-3.1.1.tar.gz"

if os.environ.has_key("CRAYOS_VERSION"):
    builder.add_option('LDFLAGS="-dynamic"')


platform = utils.get_platform()

if platform['system'] == 'Darwin':
    builder.add_option("--with-curl=/usr/lib/")

builder.build()
