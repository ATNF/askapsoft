# ASKAP Sky Model Service

## Important: Runtime errors when executing tests on Pawsey Systems
Issues with the interaction of rbuild, Python virtual environments, and the
module system cause the functional and unit tests to fail on Galaxy and related
systems. This occurs when incorrect shared libraries are loaded. 
The solution is to force the loading of the correct libraries via the
`LD_PRELOAD` environment variable.

On Galaxy, with the Python 2.7.10 module and GCC 4.9.0, the following are
required:
    export LD_PRELOAD=/pawsey/cle52up04/apps/gcc/4.3.4/python/2.7.10/lib/libpython2.7.so.1.0
    export LD_PRELOAD=$LD_PRELOAD:/opt/gcc/4.9.0/snos/lib64/libstdc++.so.6

[See Confluence](https://confluence.csiro.au/display/ASDP/Functional+Testing+with+Python) for details.

## Directory Layout

The database schema is defined by the ODB-annotated classes in the `schema`
directory. Note that the files in this directory are not used directly by the
other code. A custom build step copies the files to the datamodel directory, and
then the odb compiler generates the database schema and persistence classes.

## Compiling the database schema

If the database schema changes, either through adjusting the spreadsheets in
`./schema_definitions` or by editing the *.h files in `./schema`, then you need
to regenerate the actual C++ files used to access the target database. This is
done through the Makefile rather than rbuild, since it does not need to be
performed during every build and because compiling the odb compiler is
problematic on Pawsey systems.

First, build the odb compiler in `$ASKAP_ROOT/3rdParty/odb/odb-2.4.0/odb` with 
`rbuild -naf` to force the build despite the NO_BUILD file.

Then make the `generate` target with `make generate`. If you have changed the
data elements exposed via Ice, then you will also need to deploy the Ice
interfaces with `make generate_ice`. Don't forget to commit the Ice files to
source control.

Note that the `generate` target requires Python 3 with Pandas > 0.18 and the
xlrd package installed.

Finally, you can rebuild the SMS itself, run the tests and commit all the
changed files to source control.
