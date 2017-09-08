# build script for the Sky Model Service
# Note that the odb generated files are now added to source control.
# They are not build by rbuild. When the database schema changes, 
# the datamodel files must be regenerated with the Makefile.

from askapdev.rbuild.builders import Scons as Builder

b = Builder(".")
b.build()
