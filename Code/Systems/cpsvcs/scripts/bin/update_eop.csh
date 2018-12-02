#!/bin/csh
#
# Simple demo script to update casa leap second and IERS-A predicts.
#  4/8/2014 JER
# 12/8/2014 JER - set renew=y for IERSpredict table.
# 06/9/2014 JER - add logging, some additional robustness
#
# 07/1/2016 MAV - moved the script into "additional system-level scripts"
#                 directory to be able to deploy it with the package rather
#                 than manually. Minor adaptation for new paths. We may do
#                 this update in a different way once we move ingest to Pawsey
#
# Assumes the tables to be updated reside in subdirectories;
#
#   ./geodetic/TAI_UTC  and
#   ./geodetic/IERSpredict
#
# somewhere under the current working directory.
# Also assumes Casa utility "measuresdata" exists and 
# is pointed to in next line;
#
# Script runs asynchronously so keep some logs.
#
# Maia is the prime server, with two backup servers;
setenv IERSDIR ftp://maia.usno.navy.mil/ser7
#setenv IERSDIR ftp://toshi.nofs.navy.mil/ser7
#setenv IERSDIR ftp://cddis.gsfc.nasa.gov/pub/products/iers

# this is where the log file goes. We probably have to change it
# to /var/log/askap in the future, but someone has to think about permissions
setenv LOGFILE /tmp/update_eop.log

rm -f ${LOGFILE}.4
if ( -f ${LOGFILE}.3 ) \
  mv -f ${LOGFILE}.3 ${LOGFILE}.4
if ( -f ${LOGFILE}.2 ) \
  mv -f ${LOGFILE}.2 ${LOGFILE}.3
if ( -f ${LOGFILE}.1 ) \
  mv -f ${LOGFILE}.1 ${LOGFILE}.2
if ( -f ${LOGFILE} ) \
  mv -f ${LOGFILE} ${LOGFILE}.1
printf "Starting update_eop.csh\n" > ${LOGFILE}
#
# Standard paths on deployed system, default is usually the symlink
setenv MEASURESDATA /askap/cp/cpsvcs/default/bin/measuresdata
setenv LD_LIBRARY_PATH /askap/cp/cpsvcs/default/lib

# check measures_data directory is present, create it if not
setenv MEASURES_DIRECTORY /askap/cp/measures_data
if ( ! -e "$MEASURES_DIRECTORY" ) then
   echo "Creating $MEASURES_DIRECTORY"
   echo "Creating $MEASURES_DIRECTORY" >>& $LOGFILE
   mkdir $MEASURES_DIRECTORY
endif

cd $MEASURES_DIRECTORY
if ( $status != 0 ) then
   echo "Unable to cd into $MEASURES_DIRECTORY"
   echo "Unable to cd into $MEASURES_DIRECTORY" >>& $LOGFILE
endif
   

#
if ( ! -x "$MEASURESDATA" ) then
  echo "No executable found: $MEASURESDATA"
  echo "No executable found: $MEASURESDATA" >>& $LOGFILE
  exit
endif
#
# Look for subdirectory 'geodetic' and descend to its parent.
# If there are multiple subdirectories take the first and warn.
# 
# This is required because although the 'measuresdata' utility purports
# in its header comments to accept a runtime parameter;
#      dir[./]=<base directory for tables> 
# this appears not to be implemented, and so it must be executed from 
# the parent directory to the 'geodetic' subdirectory.
#
set GEODIR = `find . -type d -name geodetic`
@ NTARGDIRS = `echo $GEODIR | wc -w`
if ( $NTARGDIRS < 1) then
    echo "No subdirectory 'geodetic' found"
    echo "No subdirectory 'geodetic' found" >>& ${LOGFILE}
    exit
else if ( $NTARGDIRS > 1) then
    echo "Multiple 'geodetic' subdirectories found: $GEODIR"
    set GEODIR = `echo $GEODIR | awk -F\  '{print $1}'`
    echo "Arbitrarily selecting the first: $GEODIR ."
    echo "Arbitrarily selecting the first: $GEODIR ." >>& ${LOGFILE}
endif

echo "descending to $GEODIR/.."
echo "descending to $GEODIR/.." >>& ${LOGFILE}
cd "$GEODIR/.."
echo "Now in `pwd`"
echo "Now in `pwd`" >>& ${LOGFILE}
ls -Rl ./geodetic/ >>& ${LOGFILE}
#
if ( ! -d ./geodetic/TAI_UTC ) then
  echo "Directory ./geodetic/TAI_UTC missing"
  echo "Directory ./geodetic/TAI_UTC missing" >>& ${LOGFILE}
endif
if ( ! -d ./geodetic/IERSpredict ) then
  echo "Directory ./geodetic/IERSpredict missing"
  echo "Directory ./geodetic/IERSpredict missing" >>& ${LOGFILE}
endif
#
echo "Updating TAI_UTC"
echo "Updating TAI_UTC" >>& ${LOGFILE}
'rm' -f /tmp/update_
'rm' -f tai-utc.dat
wget ${IERSDIR}/tai-utc.dat >>& ${LOGFILE}
$MEASURESDATA in=tai-utc.dat type=TAI_UTC x__fn=y >>& ${LOGFILE}
#
echo "Updating IERSpredict"
echo "Updating IERSpredict" >>& ${LOGFILE}
'rm' -f finals.daily
wget ${IERSDIR}/finals.daily >>& ${LOGFILE}
$MEASURESDATA in=finals.daily type=IERSpredict renew=y x__fn=y >>& \
    ${LOGFILE}
#
# This last section will update the finals, if needed.
# Comment next line out, as required.
goto DONE
#
#@ YR4 = 1962
@ YR4 = `date -u +%Y` - 1
echo "Updating eopc04 from $YR4"
echo "Updating eopc04 from $YR4" >>& ${LOGFILE}
while ( $YR4 <= `date -u +%Y` )
  @ YR2 = $YR4 % 100
  set YR2 = `printf "%02d" $YR2`
  echo $YR2
  'rm' -f eopc04.$YR2
  wget ftp://hpiers.obspm.fr/iers/eop/eopc04/eopc04.$YR2 >>& ${LOGFILE}
  $MEASURESDATA in=eopc04.$YR2 type=IERSeop97 refresh=y x__fn=y \
      >>& ${LOGFILE}
  @ YR4 = $YR4 + 1
end
#
#@ YR4 = 1962
@ YR4 = `date -u +%Y` - 1
echo "Updating eopc04_IAU2000 from $YR4"
echo "Updating eopc04_IAU2000 from $YR4" >>& ${LOGFILE}
while ( $YR4 <= `date -u +%Y` )
  @ YR2 = $YR4 % 100
  set YR2 = `printf "%02d" $YR2`
  echo $YR2
  'rm' -f eopc04_IAU2000.$YR2
  wget ftp://hpiers.obspm.fr/iers/eop/eopc04/eopc04_IAU2000.$YR2 \
      >>& ${LOGFILE}
  $MEASURESDATA in=eopc04_IAU2000.$YR2 type=IERSeop2000 refresh=y x__fn=y \
      >>& ${LOGFILE}
  @ YR4 = $YR4 + 1
end
DONE:
ls -Rl ./geodetic/ >>& ${LOGFILE}
printf "Finished update_eop.csh\n" 
printf "Finished update_eop.csh\n" >>& ${LOGFILE}
exit
