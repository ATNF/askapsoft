#!/usr/bin/env python
# @file
#
# A matplotlib-based program to plot the stats summary file for a
# pipeline run, showing the length of time for each of the individual
# slurm tasks, colour coded by the type of job and indicating the size
# of the job in terms of number of nodes. 
#
# @copyright (c) 2018 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3 of the License,
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

import matplotlib
matplotlib.use('Agg')

import statsEntry as se
import statsCollection as sc
import statsColour as scol

import numpy as np
import pylab as plt
import os
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
    parser.add_option("-m","--metadata", dest="metadata", type="string", default="metadata",
                          help="Directory containing pipeline metadata [default: %default]")
    parser.add_option("-b","--beamlist", dest="beamlist", type="string", default="0-35",
                          help="List of beams to be included [default: %default]")
    parser.add_option("-f","--numfields", dest="numfields", type="int", default=1,
                          help="Number of fields [default: %default]")
    parser.add_option("-S","--SBID", dest="sbid", type="string", default='',
                          help="Scheduling block ID [default: %default]")
#    parser.add_option("-c","--cal", dest="showCal", action="store_true", default=False,
#                          help="Whether to show calibration jobs [default: %default]")
    parser.add_option("-v","--verbose", dest="verbose", action="store_true", default=False,
                          help="Whether to print information for each line [default: %default]")

    (options, args) = parser.parse_args()
    options.beams = beamlistToArray(options.beamlist)

    if options.sbid == '':
        print("SBID not given - you need to use the -s option")
        exit(0)
        

    # Get metadata
    sbfile = '%s/schedblock-info-%s.txt'%(options.metadata,options.sbid)
    if not os.access(sbfile,os.F_OK):
        print("Metadata file %s not found. Exiting."%sbfile)
        exit(1)
    fin = open(sbfile)
    for line in fin:
        if line.split()>0 and line.split()[0]==options.sbid:
            field=line.split()[1]
            program=line.split()[-1]
            break

#    user=os.getlogin()
    directory = os.getcwd()
        
    pipelineDate=options.statsfile.split('stats-all-')[1].split('.txt')[0]
    timelabel = 'Pipeline run commenced %s'%datetime.strptime(pipelineDate,'%Y-%m-%d-%H%M%S').strftime('%F %H:%M:%S')
#    locationlabel = 'user: %s     %s'%(user,directory)
    
#    plottitle='SBID=%s    %s    (%s)\n\n%s\n%s'%(options.sbid,field,program,timelabel,locationlabel)
    plottitle='SBID=%s    %s    (%s)\n\n%s\n%s'%(options.sbid,field,program,timelabel,directory)

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
#    plt.suptitle(options.statsfile)
    plt.suptitle(plottitle)
    plt.xlabel('Time (AWST)')
    plt.ylabel('Beam/Field')    
#    plt.show()
    ax.grid(axis='x')
    # beautify the x-labels
    plt.gcf().autofmt_xdate()
    xfmt = mdates.DateFormatter('%Y-%m-%d %H:%M')
    ax.xaxis.set_major_formatter(xfmt)
    scol.colourKey('lower right')
    #    scol.widthKey('lower right')
    outfile = 'statsPlot-%s.png'%pipelineDate
    fig.savefig(outfile)

