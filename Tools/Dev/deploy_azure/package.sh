#!/usr/bin/env bash

to_package=`cat ./bundle.list`

if [ -d ./tmp ] 
then
  echo "warn - ./tmp exists"
else
  mkdir ./tmp
fi

if [-d ./tmp/askap_apps ]
then
  echo "./tmp/askap_apps directory exists - remove to continue"
else
  mkdir ./tmp/askap_apps
fi

for app in $to_package
do
  . bundle.sh $app ./tmp/askap_apps
done
cd ./tmp
tar -cvf askap_apps.tar ./askap_apps
gzip askap_apps.tar
mv askap_apps.tar.gz ../
  
