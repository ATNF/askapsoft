# ASKAP DEPLOY
This script is only for deploying askap artefacts to the Galaxy platform at the Pawsey Supercomputing centre and is intended as
a script for admins of that system only or for use in a continuous integration environment for that platform. This script is of
no use to anyone else.

The script should be deployed to the askapops user area with suitable permissions to prevent changes and enforce private use
by askapops only (chmod 700).

*.module-template files are Modules config files for the various ASKAP modules. They contain substitution variable ${_VERSION} (the underscore to indicate that
it's not a normal env variable) which will be replaced by a valid version number when the deployment script is run.

More to come ...