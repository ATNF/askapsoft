#!/usr/bin/env python

import argparse
from casacore.tables import *
import glob

def getFreqBeam(ms):
    # Open up the SPECTRAL_WINDOW table of the current ms
    tf = table("%s/SPECTRAL_WINDOW" %(ms), readonly=True,ack=False)
    # Read the REF_FREQUENCY information
    freq = tf.getcol("REF_FREQUENCY")   
    tf.close()
    
    # Open up the msdata
    tf = table("%s/" %(ms), readonly=True,ack=False)
    # Read the FEED information
    beam = tf.getcol("FEED1")   
    tf.close()

    return freq[0],beam[0]

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Find all MSs in a nominated directory that match a given beam, and provide in frequency order')

    parser.add_argument('-d','--dir', dest='dir',required='true',help='Directory containing MSs',type=str)
    parser.add_argument('-b','--beam', dest='beam',required='true',help='Beam to match',type=int)

    args = parser.parse_args()

    mslist = glob.glob('%s/*.ms'%args.dir)
    goodlist={}
    for ms in mslist:
        freq,beam = getFreqBeam(ms)
        if beam == args.beam:
            goodlist[freq]=ms
    goodfreqs = goodlist.keys()
    goodfreqs.sort()
    for f in goodfreqs:
        print goodlist[f]
        
