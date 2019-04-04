#!/usr/bin/env python

from datetime import datetime, timedelta
from optparse import OptionParser

def readTime(timeAsString):
    main = datetime.strptime(timeAsString.split('.')[0],'%d-%b-%Y/%H:%M:%S')
    fractionalSeconds = timeAsString.split('.')[1]
    factor = 10.**len(fractionalSeconds)
    fraction = timedelta(seconds=float(fractionalSeconds)/factor)
    return main+fraction


if __name__ == '__main__':

    parser = OptionParser()
    parser.add_option("-i","--input", dest="input", type="string", default="", help="Input mslist metadata file [default: %default]")
    parser.add_option("-s","--step", dest="step", type="int", default="60", help="Approximate step time [minutes] [default: %default]")
    parser.add_option("-o","--output", dest="output", type="string", default="timerange", help="Output file with time ranges [default: %default]")
   
    (options, args) = parser.parse_args()

    if options.step <= 0:
        print("ERROR - Requested step size (%s min) is not positive"%options.step)
        exit(1)

    if len(options.output)==0:
        print("ERROR - No output filename given")
        exit(1)
    
    # Read start and end time from mslist file
    fin = open(options.input)
    for line in fin:
        if len(line.split())>8 and line.split()[4]=='Observed':
            print 'start,end = ',line.split()[6],line.split()[8]
            start = readTime(line.split()[6])
            end = readTime(line.split()[8])
            break

    

    # Find the step time to divide the range evenly
    length = (end-start).seconds + (end-start).microseconds/1.e6
    if (options.step > length):
        print("ERROR - Requested step size (%s min) is larger than observation length."%options.step)
        exit(1)
    numSteps = int( length / (options.step*60) )
    step = timedelta(seconds=length/numSteps)

    fout = open(options.output,"w")
    for t in range(numSteps):
        time = start + t * step
        newtime = time + step
        timeAsString = time.strftime('%Y-%m-%dT%H:%M:%S')+ ('%.1f'%(time.microsecond/1.e6))[1:3]
        newtimeAsString = newtime.strftime('%Y-%m-%dT%H:%M:%S')+ ('%.1f'%(newtime.microsecond/1.e6))[1:3]
        fout.write('%s %s\n'%(timeAsString, newtimeAsString))

    fout.close()
    
