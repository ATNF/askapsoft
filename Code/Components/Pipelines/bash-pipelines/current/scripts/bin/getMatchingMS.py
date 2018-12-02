#!/usr/bin/env python

import argparse
from casacore.tables import *
import glob

def getFreqBeam(ms):
    # Open up the SPECTRAL_WINDOW table of the current ms
    tf = table("%s/SPECTRAL_WINDOW" %(ms), readonly=True,ack=False)
    # Read the REF_FREQUENCY information
    freq = tf.getcol("REF_FREQUENCY")   
    nchan = tf.getcol("NUM_CHAN")   
    tf.close()
    
    # Open up the msdata
    tf = table("%s/" %(ms), readonly=True,ack=False)
    # Read the FEED information
    beam = tf.getcol("FEED1")   
    tf.close()

    return freq[0],nchan[0],beam[0]

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Find all MSs in a nominated directory that match a given beam, and provide in frequency order')

    parser.add_argument('-d','--dir', dest='dir',required='true',help='Directory containing MSs',type=str)
    parser.add_argument('-b','--beam', dest='beam',required='true',help='Beam to match',type=int)
    parser.add_argument('-c','--chanRange', dest='chanRange', required=False, help="Optional channel range on full bandwidth", type=str, default='')

    args = parser.parse_args()

    if args.chanRange != '':
        minChanReq=int(args.chanRange.split('-')[0])
        maxChanReq=int(args.chanRange.split('-')[1])

    mslist = glob.glob('%s/*.ms'%args.dir)
    goodMSlist={}
    goodNClist={}
    for ms in mslist:
        freq,nchan,beam = getFreqBeam(ms)
        if beam == args.beam:
            goodMSlist[freq]=ms
            goodNClist[freq]=nchan
    goodfreqs = goodMSlist.keys()
    goodfreqs.sort()
    chanCount=0
    for f in goodfreqs:
        nc=goodNClist[f]
        ms=goodMSlist[f]
        if args.chanRange == '' or (nc+chanCount>=minChanReq and chanCount<maxChanReq):
            if args.chanRange=='':
                cr='1-%d'%nc
            else:
                cr='%d-%d'%(max(1,minChanReq-chanCount),min(nc,maxChanReq-chanCount))
            print '%s:%s'%(ms,cr)
        chanCount = chanCount+goodNClist[f]
        
