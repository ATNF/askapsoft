# @file
# build script for AutoBuild

import glob
import os
import shutil
import subprocess
from askapdev.rbuild.builders import Scons as Builder
from askapdev.rbuild.utils import runcmd

b = Builder(".")

odb_output_dir = os.path.abspath("./datamodel")

# Set up the custom build step for generating the ODB persistence code from our
# data model
def odb_prebuild():

    # Build some important paths
    odb_dir = b.dep.get_install_path("odb")
    libodb_dir = b.dep.get_install_path("libodb")
    libodb_boost_dir = b.dep.get_install_path("libodb-boost")
    boost_dir = b.dep.get_install_path("boost")

    odb_compiler = os.path.join(odb_dir, "bin/odb")
    schema_dir = os.path.abspath("./schema")
    odb_includes = os.path.join(libodb_dir, "include")
    odb_boost_includes = os.path.join(libodb_boost_dir, "include")
    boost_includes = os.path.join(boost_dir, "include")

    print('odb: ' + odb_compiler)

    # build the command
    cmd = [
        odb_compiler,  # The ODB compiler
        '--generate-query',  # Generate query support code
        '--generate-schema',  # Generate database schema
        '--schema-format', 'embedded',
        '--schema-format', 'sql',
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

    # I want a failed ODB compile to abort the whole build. Using check_call with
    # the raised exception on failure seems the simplest way to achieve this.
    return subprocess.check_call(cmd)


b.add_precallback(odb_prebuild)
b.add_extra_clean_targets(odb_output_dir)
b.build()
