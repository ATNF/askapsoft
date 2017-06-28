#!/bin/bash -l
#
#  A script to make a new ACESOPS release
#  This does the following:
#    * Checks out the ACES subversion to a temporary location
#    * Extracts the revision number
#    * Moves the temporary directory to one labelled by the revision
#      number
#    * Copies the template module file to the correct location, and
#      updates the REVISION number therein.
#    * Sets permissions etc correctly for the ACES directory and the
#      module file.
#  It does NOT make this new release the default version - that should
#  be a deliberate step by the askapops user.

if [ "$(whoami)" != "askapops" ]; then
    echo "This should ONLY be done by the askapops user. That is not you!"
    exit 1
fi

releaseDir=/group/askap/acesops
moduleDir=/group/askap/modulefiles/acesops
URL="https://svn.atnf.csiro.au/askap/ACES"

revision=0
while getopts ':r:h' opt
do
    case $opt in
	r) revision=$OPTARG;;
        h) echo "Usage: $USAGE"
           exit 0;;
	\?) echo "ERROR: Invalid option: $USAGE"
	    exit 1;;
    esac
done

if [ $revision -eq 0 ]; then
    revision=$(svn info $URL | grep Revision | awk '{print $2}')
fi
newRepo=ACES-r${revision}

mkdir -p releaseDir
mkdir -p moduleDir

cd $releaseDir
svn co -r $revision $URL $newRepo

chgrp -R askap $newRepo
find $newRepo -type f | xargs chmod oug+r
find $newRepo -type d | xargs chmod oug+rx

cd $moduleDir
cat > r${revision} <<EOF
#%Module1.0#####################################################################
##
## acesops modulefile
##
proc ModulesHelp { } {
        global version

        puts stderr "\tThis modules provides a fixed version of the ACES subversion repository"

}

# No two versions of this module can be loaded simultaneously
conflict        acesops


prereq aces
if { [ module-info mode load ] } {
    module load aces
}

module-whatis   "ASKAPsoft pipeline processing"

set REVISION  $revision
set ACESHOME  /group/askap/acesops/ACES-r\$REVISION

setenv ACES \$ACESHOME
setenv ACES_VERSION r\$REVISION

EOF

chmod oug+r r${revision}
chgrp askap r${revision}

