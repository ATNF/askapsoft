#!/bin/bash -l
#
# Test program to trial the parsing of measurement sets via mslist to
# extract the list of fields and spectral windows
#
# @copyright (c) 2017 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# @author Matthew Whiting <Matthew.Whiting@csiro.au>
#

mslist=$(echo "$1" | sed -e 's/,/ /g')

spwList=tmpspw
fieldList=tmpfield
listfile=tmplist

for ms in "$@"; do
    echo "Parsing $ms"
    mslist --full "$ms" 1>& "$listfile"
    nsp=$(grep Spectral "$listfile" | head -1 | cut -d'(' -f 2 | cut -d ' ' -f 1)
    npol=$(grep Spectral "$listfile" | head -1 | cut -d'(' -f 2 | cut -d ' ' -f 6)
    nspw=$(echo "$nsp" "$npol" | awk '{print $1*$2}')
    grep -A"${nspw}" SpwID "$listfile" | tail -n "${nspw}" | cut -f 4- >> "$spwList"

    nfields=$(grep Fields "$listfile" | head -1 | cut -f 4- | cut -d' ' -f 2)
    grep -A"${nfields}" RA "$listfile" | tail -n "${nfields}" | cut -f 4- >> "$fieldList"
    
done

nSpw=$(sort -k7n "$spwList" | uniq | wc -l)

doMerge=false
if [ "$nSpw" -gt 1 ]; then
    doMerge=true
fi

fieldnames=$(sort -k2 $fieldList | uniq | awk '{print $2}')
nfieldnames=$(sort -k2 $fieldList | uniq | wc -l)

echo "======="
echo "List of MSs has $nSpw spectral windows"
if [ $doMerge == true ]; then
    echo "  They are:"
    sort -k7n "$spwList" | uniq
    echo "   --> we will need to run msmerge to combine them"
fi
echo "There are $nfieldnames fields in this list:"
#sort -k2 $fieldList | uniq
echo "List of fieldnames and positions is:"

# Set the args for footprint: summary output, name, band, PA, pitch
. "$PIPELINEDIR/defaultConfig.sh"
footprintArgs="-t"
footprintArgs="$footprintArgs -n $BEAM_FOOTPRINT_NAME"
footprintArgs="$footprintArgs -b $FREQ_BAND_NUMBER"
footprintArgs="$footprintArgs -a $BEAM_FOOTPRINT_PA"
footprintArgs="$footprintArgs -p $BEAM_PITCH"

for field in $fieldnames; do
    ra=$(grep "$field" "$fieldList" | awk '{print $3}')
    dec=$(grep "$field" "$fieldList" | awk '{print $4}')
    dec=$(echo "$dec" | awk -F'.' '{printf "%s:%s:%s.%s",$1,$2,$3,$4}')
    echo "    $field  $ra $dec"
    footprintOut="$parsets/footprintOutput-${NOW}-${field}.txt"
    #    footprint.py $footprintArgs -r '$ra,$dec' > ${footprintOut}
    footprint.py $footprintArgs -r "$ra,$dec"
done

rm  -f $spwList $fieldList $listfile
