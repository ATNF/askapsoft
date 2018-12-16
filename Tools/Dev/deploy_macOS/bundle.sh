#!/usr/bin/env bash 


FIND_DYLIB_CMD="otool -L "
CHANGE_RPATH_CMD="install_name"

echo "====================================="
echo "info - bundle script for applications"
echo "====================================="


function update_rpath_in_lib () {

  dylib_tmp=$1

  if [[ $dylib_tmp =~ ^/ ]]
  then 
    full_path_present=1
  else
    full_path_present=0
  fi

  dylib=$(basename $dylib_tmp)
  echo "loooking for $dylib"
  FULL_DYLIB_PATH=`find . -name $dylib | head -1 | awk '{print $1}'`
  
  if [ -z "$FULL_DYLIB_PATH" ] 
  then
    echo "NOT FOUND"
  else
    echo "found $FULL_DYLIB_PATH "
    cp $FULL_DYLIB_PATH $BUNDLE_LIBS
    echo "Updating DYLIB to use rpath"
    install_name_tool -id @rpath/$dylib $BUNDLE_ROOT/libs/$dylib

   
    echo "Changing binary to look for dylib at rpath"

    if [[ $full_path_present == 1 ]]
    then
      install_name_tool -change $dylib_tmp  @rpath/$dylib $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
    else
      install_name_tool -change $dylib  @rpath/$dylib $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
    fi

    echo "Done"
    
  fi
 
}
       
APP=$1

if [ -z "$APP" ]
then
      echo "Specifiy APP to bundle on the command line"
      exit 1;
else
      echo "will attempt to bundle $APP";
fi
echo "info - moving to root directory"

cd ../../../

echo "info - at $PWD"

FULL_PATH_TO_APP=`find . -name $APP | head -1`;

if [ -z "$FULL_PATH_TO_APP" ]
then
   echo "Cannot find $APP - are you sure?"
   exit 1
else
   echo "Found $FULL_PATH_TO_APP"
fi
BUNDLE_ROOT=/tmp/$APP

echo "info - creating tmp directory for bundle"

if [ -d $BUNDLE_ROOT ] 
then
  rm -rf $BUNDLE_ROOT
fi
 
mkdir $BUNDLE_ROOT

if [ $? -eq 0 ]; then
    echo "info - created tmp dir OK"
else
    echo "FAIL in creating tmp dir"
    exit 1
fi

BUNDLE_LIBS=$BUNDLE_ROOT/libs
mkdir $BUNDLE_LIBS

cp $FULL_PATH_TO_APP $BUNDLE_ROOT


DYLIBS=`$FIND_DYLIB_CMD $FULL_PATH_TO_APP | awk '{print $1}'`
for dylib_tmp in $DYLIBS
do
  if [[ $dylib_tmp =~ ^/ ]]
  then 
    full_path_present=1
  else
    full_path_present=0
  fi

  dylib=$(basename $dylib_tmp)
  echo "loooking for $dylib"
  FULL_DYLIB_PATH=`find . -name $dylib | head -1 | awk '{print $1}'`
  
  if [ -z "$FULL_DYLIB_PATH" ] 
  then
    echo "NOT FOUND"
  else
    echo "found $FULL_DYLIB_PATH "
    cp $FULL_DYLIB_PATH $BUNDLE_LIBS
    echo "Updating DYLIB to use rpath"
    install_name_tool -id @rpath/$dylib $BUNDLE_ROOT/libs/$dylib

   
    echo "Changing binary to look for dylib at rpath"

    if [[ $full_path_present == 1 ]]
    then
      install_name_tool -change $dylib_tmp  @rpath/$dylib $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
    else
      install_name_tool -change $dylib  @rpath/$dylib $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
    fi

    echo "Done"
    
  fi
 
done
echo "Actually add the rpath to the binary"
install_name_tool -add_rpath @executable_path/libs $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
