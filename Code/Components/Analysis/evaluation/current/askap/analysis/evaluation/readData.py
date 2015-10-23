#!/usr/bin/env python

## @file
#  A file containing functions to read in data ready for plotting. It
#  can read data for matching sources and sources that didn't match.

## @namespace readData
#  Data I/O for python scripts.
#  A set of functions to read in the results of the pattern matching,
#  so that we can produce nice plots showing how well the source list
#  matches the reference list.

from pkg_resources import require
require('numpy')
require('matplotlib')
from pylab import *
from numpy import *
from askap.analysis.evaluation.modelcomponents import *
import logging

#############################################################################

## @ingroup plotting
# Reads the set of matches, returning three dictionaries: the source
# and reference dictionaries (ID matched to the appropriate object),
# plus a dictionary matching IDs of matched objects
def readMatches(matchfile,srcDict,refDict):

    fin=open(matchfile)
    matchlist=[]
    for line in fin:
        cols=line.split()
        srcID=cols[1]
        refID=cols[2]
        if srcDict.has_key(srcID) and refDict.has_key(refID):
            matchlist.append(Match(line,srcDict,refDict))
        else:
            if not srcDict.has_key(srcID):
                logging.error("Src key %s is missing from src dictionary"%srcID)
            if not refDict.has_key(refID):
                logging.error("Ref key %s is missing from ref dictionary"%refID)

    return matchlist

def readMisses(missfile,catalogue,key):
    fin=open(missfile)
    misslist=[]
    for line in fin:
        cols=line.split()
        if cols[0]==key:
            id = cols[1]
            if catalogue.has_key(id):
                misslist.append(catalogue[id])
            else:
                logging.error("Key %s is missing from dictionary"%id)

    return misslist


## @ingroup plotting
# Reads a catalogue of objects
def readCat(filename,catalogueType):

    catDict={}
    if(catalogueType == "Selavy" or
       catalogueType=="Continuum" or
       catalogueType=="ContinuumID" or
       catalogueType=="SUMSS" or
       catalogueType=="NVSS"):

        fin=open(filename)
        for line in fin:
            if(line[0]!='#'):
                if catalogueType=="Selavy":
                    obj=SelavyObject(line)
                elif catalogueType=="Continuum":
                    obj=ContinuumObject(line)
                elif catalogueType=="ContinuumID":
                    obj=ContinuumIDObject(line)
                elif catalogueType=="SUMSS":
                    obj=SUMSSObject(line)
                elif catalogueType=="NVSS":
                    obj=NVSSObject(line)
                catDict[obj.id] = obj
        fin.close()

    else:
        logging.error("Catalogue type %s not known"%catalogueType)

    return catDict

