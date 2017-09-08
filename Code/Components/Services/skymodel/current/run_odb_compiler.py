# Build script (converted from an rbuild build.py to a standalone script) for
# building the database files from the schema.  Note that you must build the odb
# compiler first (in ~/code/askapsoft/3rdParty/odb/odb-2.4.0/odb) with `rbuild
# -f` so that it ignores the NO_BUILD file.  This is only required when the
# database schema files in ./schema or ./schema_definitions have changed.  The
# odb-generated files in ./datamodel should be added to source control.

import glob
import os
import pprint
import shutil
import subprocess
import sys


thirdparty_dir = os.path.expandvars('$ASKAP_ROOT/3rdParty/')
odb_base_dir = os.path.join(thirdparty_dir, 'odb/odb-2.4.0')
odb_output_dir = os.path.abspath('./datamodel')

# Set up the custom build step for generating the ODB persistence code from our
# data model
def run_odb_compiler():

    # Build some important paths
    odb_dir = os.path.join(odb_base_dir, 'odb/install')
    libodb_dir = os.path.join(odb_base_dir, 'libodb/install')
    libodb_boost_dir = os.path.join(odb_base_dir, 'libodb-boost/install')
    boost_dir = os.path.join(thirdparty_dir, 'boost/boost-1.56.0/install')

    odb_compiler = os.path.join(odb_dir, 'bin/odb')
    schema_dir = os.path.abspath('./schema')
    odb_includes = os.path.join(libodb_dir, 'include')
    odb_boost_includes = os.path.join(libodb_boost_dir, 'include')
    boost_includes = os.path.join(boost_dir, 'include')

    print('odb: ' + odb_compiler)

    # build the command
    cmd = [
        odb_compiler,  # The ODB compiler
        '--generate-query',  # Generate query support code
        '--generate-schema',  # Generate database schema
        '--schema-format', 'embedded',
        '--schema-format', 'sql',
        '-v',  # verbose output
        '--output-dir', odb_output_dir,  # output location for generated files
        '-I', boost_includes,  # Boost headers
        '-I', odb_includes,  # ODB headers
        '-I', odb_boost_includes,  # ODB boost headers
        '--cxx-suffix', '.cc',  # Use the ASKAP default .cc instead of .cxx
        '--hxx-suffix', '.h',  # Use the ASKAP default .h instead of .hxx
        '--ixx-suffix', '.i',  # Use .i instead of .ixx
        '--std', 'c++98',  # Generate C++98 compliant code. Other options are c++11, c++14.
        # Generate support code for both sqlite and mysql. Specific instance
        # will be selected from the parset at runtime. Note that for production
        # builds, it will be slightly more efficient to disable multi-database
        # support, and only generate code for the production database.
        '--multi-database', 'dynamic',
        '--database', 'common',  # Generate the database-agnostic code
        '--database', 'sqlite',
        '--database', 'mysql',
        # '--database', 'pgsql',
        # For PostgreSQL, ODB needs to know the version. If this is left out, then
        # exceptions will occur during schema creation:
        # ERROR: relation "schema_version" already exists
        # specifying the version lets ODB generate the correct schema
        # '--pgsql-server-version', '9.4',
        # enable the ODB profile for boost smart pointers and date-times
        '--profile', 'boost/smart-ptr',
        '--profile', 'boost/date-time/posix-time',
        ]

    # append the list of sources
    sources = glob.glob(os.path.join(schema_dir, "*.h"))
    schema_sources = glob.glob(os.path.join(schema_dir, "*.i"))
    cmd.extend(sources)

    if not os.path.exists(odb_output_dir):
        os.mkdir(odb_output_dir)

    # even though odb supports compilation from separate source and output
    # directories, it generates code that expects the input headers to be in the
    # same directory. Sigh...
    for src in sources + schema_sources:
        dst = os.path.join(odb_output_dir, os.path.basename(src))
        shutil.copy(src, dst)

    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    while True:
        nextline = process.stdout.readline().decode()
        if nextline == '' and process.poll() is not None:
            break
        sys.stdout.write(nextline)
        sys.stdout.flush()

    output = process.communicate()[0].decode()
    exitCode = process.returncode

    if (exitCode == 0):
        return output
    else:
        raise ProcessException(command, exitCode, output)


if __name__ == '__main__':
    print('Running odb ...')
    run_odb_compiler()
