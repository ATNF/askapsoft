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
