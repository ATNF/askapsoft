#!/usr/bin/env bash 


FIND_DYLIB_CMD="ldd "

echo "====================================="
echo "info - bundle script for applications"
echo "====================================="


    
APP=$1
BUNDLE_ROOT=$2

if [ -z "$APP" ]
then
      echo "Specifiy APP to bundle on the command line"
      exit 1;
else
      echo "will attempt to bundle $APP";
fi
echo "info - moving to root directory"

pushd .

cd ../../../

echo "info - at $PWD"

FULL_PATH_TO_APP=`find . -name $APP | grep "install/bin" `;

if [ -z "$FULL_PATH_TO_APP" ]
then
   echo "Cannot find $APP - are you sure?"
   exit 1
else
   echo "Found $FULL_PATH_TO_APP"
fi

if [ -z "$BUNDLE_ROOT" ] 
then
   BUNDLE_ROOT=/tmp/$APP
fi


echo "info - creating tmp directory for bundle"

if [ -d $BUNDLE_ROOT ] 
then
  echo "Already exists"
else
  mkdir $BUNDLE_ROOT
fi
 
BUNDLE_LIBS=$BUNDLE_ROOT/libs

if [ -d $BUNDLE_LIBS ] 
then
  echo "Already exists"
else
  mkdir $BUNDLE_LIBS
fi
 
function update_rpath_in_lib () {
	full_path=`find . -name $1 | head -1`
	if [ -z "$full_path" ] 
	then
		#echo "Cannot find $1 - must be system"
		echo "0"
	else
		#echo "Found $1 at $full_path"
		if [ -f "$BUNDLE_LIBS/$(basename $full_path)" ]
		then
			echo "0"

		else
		  	cp $full_path $BUNDLE_LIBS/$(basename $full_path)
			patchelf --set-rpath '$ORIGIN' $BUNDLE_LIBS/$(basename  $full_path)
			echo "1"
		fi

	fi
}
cp $FULL_PATH_TO_APP $BUNDLE_ROOT

echo "Actually add the rpath to the binary"
patchelf --set-rpath '$ORIGIN/libs' $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
added_lib=1
while [ $added_lib -eq 1 ] 
do
        added_lib=0
	echo "LOOP"
	DYLIBS=`$FIND_DYLIB_CMD $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP) | awk '{print $1}'`
	for dylib_tmp in $DYLIBS
	do     
		echo "Looking for $dylib_tmp"
		added_lib_tmp=$(update_rpath_in_lib $dylib_tmp) 
		echo "returns $added_lib_tmp"
		if [ $added_lib_tmp -eq 1 ]
		then
			added_lib=1
		fi
	done

done
popd

