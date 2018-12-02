# ODB C++ Object-Relational Mapping Compiler

This directory builds the ODB compiler, which is required to generate the C++
source for database integration. The odb tool requires g++ to be compiled with
the `--enable-plugin` flag. Additionally the g++ version must be >= 4.5.

As compiling odb is problematic on Pawsey systems, builds are disabled via the
NO_BUILD file.

The odb compiler is only required when updating or changing the database schema
for a C++ database application. It compiles `#pragma` annotated C++ classes into
the actual classes for compilation. These generated classes should be added to
source control and then only updated as required for schema changes.

Doing so requires building this directory on supported platforms (such as
Debian) by using the `--force` flag to rbuild, forcing it to ignore the NO_BUILD
file. The database integration classes can then be recompiled and committed to
source control. For an example of this, see the Sky Model Service.
