from askapdev.rbuild.builders import Setuptools as Builder
import askapdev.rbuild.utils as utils

builder = Builder()
builder.remote_archive = "pywcs-1.12.tar.gz"
builder.nowarnings = True

platform =  utils.get_platform()
if platform['system'] == 'Darwin':
  builder.add_option('CFLAGS="-framework Accelerate"')

builder.build()
