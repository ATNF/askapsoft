#!/usr/bin/env python

import matplotlib
matplotlib.use('Agg')

import statsEntry as se
import statsCollection as sc
import statsColour as scol

import numpy as np
import pylab as plt
from datetime import datetime, timedelta
#import matplotlib.patches as patches
import matplotlib.dates as mdates
from optparse import OptionParser

def beamlistToArray(beamlist):
    beams=np.array([])
    for b1 in beamlist.split(','):
        start=int(b1.split('-')[0])
        end=int(b1.split('-')[-1])
        for b2 in range(start,end+1):
            beams=np.append(beams,b2)
    return beams


#############
if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option("-s","--statsfile", dest="statsfile", type="string", default="",
                          help="Input stats file [default: %default]")
    parser.add_option("-b","--beamlist", dest="beamlist", type="string", default="0-35",
                          help="List of beams to be included [default: %default]")
    parser.add_option("-f","--numfields", dest="numfields", type="int", default=1,
                          help="Number of fields [default: %default]")
#    parser.add_option("-c","--cal", dest="showCal", action="store_true", default=False,
#                          help="Whether to show calibration jobs [default: %default]")
    parser.add_option("-v","--verbose", dest="verbose", action="store_true", default=False,
                          help="Whether to print information for each line [default: %default]")

    (options, args) = parser.parse_args()
    options.beams = beamlistToArray(options.beamlist)

    fin = open(options.statsfile,'r')

    coll = sc.StatsCollection(options)
    
    for line in fin:
        if line[0] != '#' and line.split()[0]!='JobID':# and line.split()[4]!='---':

            #print line
            entry = se.StatsEntry(line,options)
            #print entry.label
            if entry.valid():
                coll.addEntry(entry)

    fig=plt.figure(1,figsize=(16,9))
    plt.xticks(rotation=30)
    ax=fig.add_subplot(111)
    coll.plot(ax)
    plt.tick_params(direction='out', which='both', width=1)
    plt.suptitle(options.statsfile)
    plt.xlabel('Time (AWST)')
    plt.ylabel('Beam/Field')    
#    plt.show()
    ax.grid(axis='x')
    # beautify the x-labels
    plt.gcf().autofmt_xdate()
    xfmt = mdates.DateFormatter('%d-%m-%y %H:%M')
    ax.xaxis.set_major_formatter(xfmt)
    scol.colourKey('lower right')
    #    scol.widthKey('lower right')
    fig.savefig('statsPlot-%s.png'%options.statsfile)

