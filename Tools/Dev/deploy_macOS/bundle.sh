#!/usr/bin/env bash 


FIND_DYLIB_CMD="otool -L "
CHANGE_RPATH_CMD="install_name"

echo "====================================="
echo "info - bundle script for applications"
echo "====================================="


function update_rpath_in_target () {

  dylib_tmp=$1
  target=$2


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
      install_name_tool -change $dylib_tmp  @rpath/$dylib $BUNDLE_ROOT/$(basename $target)
    else
      install_name_tool -change $dylib  @rpath/$dylib $BUNDLE_ROOT/$(basename $target)
    fi

    echo "Done"
  fi
}
function update_depends {
    target=$1 
    # now some recursive checking
    DEPENDS=`$FIND_DYLIB_CMD $target | awk '{print $1}'`    
    for depend_tmp in $DEPENDS
    do 
      depend_name=$(basename $depend_tmp)
      echo "working on $depend_tmp"
      #check whether this has a path attached  
      if [[ $depend_tmp =~ ^/ ]]
      then
        #must check whether this is a System or ASKAP library.
        # try to find it in the code tree
        if [ -f $BUNDLE_LIBS/$depend_name ] 
        then 
          echo "ASKAP LIB WITH FULL PATH"
          install_name_tool -change $depend_tmp  @rpath/$depend_name $BUNDLE_ROOT/$(basename $target)
        else
          echo "NOT ASKAP LIB"
        fi

      # check if we are alread rpath
      elif [[ $depend_tmp =~ ^@ ]]
         then
         echo "RPATH already in name"
      else
        echo "changing $depend_name to  @rpath/$depend_name in $BUNDLE_ROOT/libs/$dylib"
        install_name_tool -change $depend_name  @rpath/$depend_name $BUNDLE_ROOT/libs/$dylib
      fi

    done
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
  update_rpath_in_target $dylib_tmp $FULL_PATH_TO_APP
done

INSTALLED_DYLIBS=`ls -1 $BUNDLE_LIBS/`
for dylib in $INSTALLED_DYLIBS
do
  update_depends $BUNDLE_LIBS/$dylib
done

echo "Actually add the rpath to the binary"
install_name_tool -add_rpath @executable_path/libs $BUNDLE_ROOT/$(basename $FULL_PATH_TO_APP)
