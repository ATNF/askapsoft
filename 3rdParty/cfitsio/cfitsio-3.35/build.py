from askapdev.rbuild.builders import Autotools as Builder

builder = Builder("cfitsio", buildtargets=['shared'])
builder.remote_archive = "cfitsio3350.tar.gz"
builder.nowarnings = True
builder.add_option("--enable-reentrant")
builder.add_option("--enable-sse2")
builder.build()
