from askapdev.rbuild.builders import Autotools as Builder

builder = Builder(buildtargets=['busyfit','lib'])
builder.remote_archive = "busyfit-0.2.tar.gz"

gsl = builder.dep.get_install_path("gsl")

builder.add_option("--with-gsl=%s" % gsl)

builder.add_file("files/configure.ac")
builder.add_file("files/configure")
builder.add_file("files/Makefile.in")
builder.add_file("files/install-sh")
builder.add_file("files/config.sub")
builder.add_file("files/config.guess")

builder.build()
