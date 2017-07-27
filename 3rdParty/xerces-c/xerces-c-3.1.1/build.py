import os
from askapdev.rbuild.builders import Autotools as Builder
import askapdev.rbuild.utils as utils

builder = Builder()
builder.remote_archive = "xerces-c-3.1.1.tar.gz"

is_cray = False
if os.environ.has_key("CRAYOS_VERSION"):
    builder.add_option('LDFLAGS="-dynamic"')
    is_cray = True

platform = utils.get_platform()

# On Debian, we appear to require the Gnu unicode transcoder rather than the
# default ICU library (Debian 8 at least has the wrong version of ICU as a
# system package).
if platform['system'] == 'Linux' and not is_cray:
    builder.add_option("--enable-transcoder-gnuiconv")

if platform['system'] == 'Darwin':
    builder.add_option("--with-curl=/usr/")
    builder.add_option("--enable-transcoder-macosunicodeconverter")
    builder.add_option("--enable-netaccessor-cfurl")
    builder.add_option("--enable-netaccessor-cfurl")
    builder.add_option('CFLAGS="-arch x86_64"')
    builder.add_option('CXXFLAGS="-arch x86_64"')


builder.build()
