#!/usr/bin/env bash

mslist=`echo $1 | sed -e 's/,/ /g'`

spwList=tmpspw
fieldList=tmpfield
listfile=tmplist

for ms in $@; do
    echo "Parsing $ms"
    mslist --full $ms 1>& $listfile
    nsp=`grep Spectral $listfile | cut -d'(' -f 2 | cut -d ' ' -f 1`
    npol=`grep Spectral $listfile | cut -d'(' -f 2 | cut -d ' ' -f 6`
    nspw=`echo $nsp $npol | awk '{print $1*$2}'`
    grep -A${nspw} SpwID $listfile | tail -n ${nspw} | cut -f 4- >> $spwList

    nfields=`grep Fields $listfile | cut -f 4- | cut -d' ' -f 2`
    grep -A${nfields} RA $listfile | tail -n ${nfields} | cut -f 4- >> $fieldList
    
done

nSpw=`sort -k7n $spwList | uniq | wc -l`

doMerge=false
if [ $nSpw -gt 1 ]; then
    doMerge=true
fi

fieldnames=`sort -k2 $fieldList | uniq | awk '{print $2}'`
nfieldnames=`sort -k2 $fieldList | uniq | wc -l`

echo "======="
echo "List of MSs has $nSpw spectral windows"
if [ $doMerge == true ]; then
    echo "  They are:"
    sort -k7n $spwList | uniq
    echo "   --> we will need to run msmerge to combine them"
fi
echo "There are $nfieldnames fields in this list:"
sort -k2 $fieldList | uniq
echo "List of fieldnames is:"
for field in $fieldnames; do
    echo "    $field"
done

rm  -f $spwList $fieldList $listfile
