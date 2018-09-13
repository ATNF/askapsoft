#!/usr/bin/env python

import sys 
import argparse
from casacore.tables import *

"""
Code for getting metadata info from msdata.
metadata currently output include: 
	1. beamID (FeedID in casa)
	2. Frequency information
The need for this code arose when ingest 
had to write msdata split-by-beam-and-frequency. 
The ordering of the files apparently was not 
straightforward, and so the split out files 
could not be named with tags having the BeamID 
and the frequency bandID. The pipeline therefore 
needs to query metadata in the msfiles to be able 
to correctly associate the beam-frequency data. 
Issue: The "mslist" app (not regarded as part of 
ASKAPsoft, but available) is a quicklook tool 
for querying metadata. However not all fields 
can be queried using it. 
Resolution: Write a python-casacore script to 
query metadata that "mslist" can or cannot 
currently read and output. 


                                     --wr, 23 Aug, 2018

"""

def parse_args():
    """
    Parse input arguments
    """
    parser = argparse.ArgumentParser(description='Query and print metadata information from a measurement set.')

    parser.add_argument('-m','--msdata', dest='ms_data',required='true',help='Input msdata (with path)',
                        type=str)
    parser.add_argument('-q','--query', dest='query_type',required='true',choices=["beam","freq"],help='Field to query',
                        type=str)

    if len(sys.argv) < 2: 
        parser.print_usage()
        sys.exit(1)

    args = parser.parse_args()
    return args


if __name__ == "__main__":
    args = parse_args()

    ms = args.ms_data
    query = args.query_type
    # Open up the SPECTRAL_WINDOW table of the current ms
    if query == "freq":
	    tf = table("%s/SPECTRAL_WINDOW" %(ms), readonly=True,ack=False)
	    # Read the REF_FREQUENCY information
	    rFreq = tf.getcol("REF_FREQUENCY")   
	    #print "REFERENCE_FREQUENCY: ",rFreq[0]
	    print rFreq[0]
	    # Close the ms
	    tf.close()
    elif query == "beam":
	    # Open up the msdata
	    tf = table("%s/" %(ms), readonly=True,ack=False)
	    # Read the FEED information
	    beamNum = tf.getcol("FEED1")   
	    #print "BEAM_ID: ",beamNum[0]
	    print beamNum[0]
	    # Close the ms
	    tf.close()
    else: 
	    print "Invalid query parameter. See help."
