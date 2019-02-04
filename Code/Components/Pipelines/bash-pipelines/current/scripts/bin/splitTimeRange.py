#!/usr/bin/env python

from datetime import datetime, timedelta
from optparse import OptionParser

def readTime(timeAsString):
    main = datetime.strptime(timeAsString.split('.')[0],'%d-%b-%Y/%H:%M:%S')
    fraction = timedelta(seconds=float(timeAsString.split('.')[1])/10.)
    return main+fraction


if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option("-i","--input", dest="input", type="string", default="", help="Input mslist metadata file [default: %default]")
    parser.add_option("-s","--step", dest="step", type="int", default="30", help="Approximate step time [minutes] [default: %default]")
    (options, args) = parser.parse_args()

    # Read start and end time from mslist file
    fin = open(options.input)
    for line in fin:
        if len(line.split())>8 and line.split()[4]=='Observed':
            start = readTime(line.split()[6])
            end = readTime(line.split()[8])
            break

    # Find the step time to divide the range evenly
    length = (end-start).seconds + (end-start).microseconds/1.e6
    numSteps = int( length / (options.step*60) )
    step = timedelta(seconds=length/numSteps)

    for t in range(numSteps+1):
        time = start + t * step
        newtime = time + step
        timeAsString = time.strftime('%d-%b-%Y/%H:%M:%S')+ ('%.1f'%(time.microsecond/1.e6))[1:3]
        newtimeAsString = newtime.strftime('%d-%b-%Y/%H:%M:%S')+ ('%.1f'%(newtime.microsecond/1.e6))[1:3]
        print timeAsString, newtimeAsString
    
