# ASKAP Sky Model Service

## Directory Layout

The database schema is defined by the ODB-annotated classes in the `schema`
directory. Note that the files in this directory are not used directly by the
other code. A custom build step copies the files to the datamodel directory, and
then the odb compiler generates the database schema and persistence classes.
