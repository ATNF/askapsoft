import os

from askapdev.rbuild.builders import Autotools as Builder

builder = Builder("cfitsio", buildtargets=['shared'])
builder.remote_archive = "cfitsio3420.tar.gz"
#builder.nowarnings = True

if os.path.basename(os.getenv('FC')) == "pgfortran":
    builder.add_env("CPPFLAGS", "-DpgiFortran")

builder.build()
