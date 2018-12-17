This directory contains the resources for deploying to the Azure Cloud for the purposes of building

It also contains some packaging code. This puts together a list of binaries (specified in bundle.ini) together with all their local (ASKAP) dependencies into a compressed tar ball.

File list:

askap_build.sh - script to boostrap and build the repo
bundle.ini - list of apps to bundle
bundle.sh - will bundle an app
package.sh - will read the bundle.ini and package all the apps up using bundle and generate a basic debian binary package and a tarball
build-provision.sh - all the provision lines to bring up the Azure UbuntuLTS (bionic) image to a state where askap can be built
run-provision.sh - all the provision lines required to bring up the Azure UbuntuLTS (bionic) image to a state where the apps can be run




