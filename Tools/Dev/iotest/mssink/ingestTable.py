#!/usr/bin/env python
#
# ingestPlot.py
#
# TODO: Update the description below
#
# This is a simple plotter for the data coming out of tMSSink, the ingest MS writer simulation app
# that Max wrote to test the write IO issues we are having with scratch2.
#
# It reads in a file created by the tMSSink program, extracts the timing data out of it
# and plots the data.
#
# Y axis is write time in seconds
# X axis is the integration ordinal
#
# The data comes in the form:
#
# INFO  askap.tMSSink (0, galaxy-ingest07) [2016-08-29 16:42:15,453] - Received 763 integration(s) for rank=0
# INFO  askap.tMSSink (0, galaxy-ingest07) [2016-08-29 16:42:17,169] -    - mssink took 1.99 seconds
#
# Lines of the first type are discarded and the seconds value in the second line type are used as data for plot.
# regex is used to find and extract that value.
#
# So far the file processed is hard coded.
#
# Copyright: CSIRO 2017
# Author: Max Voronkov <maxim.voronkov@csiro.au>(original)
# Author: Eric Bastholm <eric.bastholm@csiro.au>
# Author: Paulus Lahur <paulus.lahur@csiro.au>
# Author: Stephen Ord <stephen.ord@csiro.au> (just added the table)

# import python modules
import argparse
import re
import numpy as np
import matplotlib
# Use non-interactive backend to prevent figure from popping up
# Note: Agg = Anti Grain Geometry
matplotlib.use('Agg')
import matplotlib.pyplot as pl

#for the pretty print tables
import subprocess
import tempfile
import html


def pprinttable(rows,htmlFile):
    esc = lambda x: html.escape(str(x))
    sour = "<table border=1>"
    if len(rows) == 1:
        for i in range(len(rows[0]._fields)):
            sour += "<tr><th>%s<td>%s" % (esc(rows[0]._fields[i]), esc(rows[0][i]))
    else:
        sour += "<tr>" + "".join(["<th>%s" % esc(x) for x in rows[0]._fields])
        sour += "".join(["<tr>%s" % "".join(["<td>%s" % esc(y) for y in x]) for x in rows])
    with open(htmlFile,"ab") as f:
        f.write(sour.encode("utf-8"))

        imageline = "\n</table>\n<img src=\"" + plotFile + "\"></img>"
        f.write(imageline.encode("utf-8"))
        f.flush()
        print(
            subprocess
            .Popen(["w3m","-dump",f.name], stdout=subprocess.PIPE)
            .communicate()[0].decode("utf-8").strip()
        )

from collections import namedtuple



## End the pretty print table
# find all lines that have "mssink took" followed by a group of the form n.nn

# glob for all the log files
import glob
namelist = []
for name in glob.glob('*.log'):
    namelist.append(name)

import os


parser = argparse.ArgumentParser(
    description="Combine all the results of run_tests.sh",
    formatter_class=argparse.ArgumentDefaultsHelpFormatter)
resultFileDefault = "results.html"
parser.add_argument("-r", "--resultfile", help="Output html file", default=resultFileDefault)
args = parser.parse_args()
resultFile = args.resultfile
htmlFile = os.path.splitext(resultFile)[0] + ".html"

with open (htmlFile,"w") as f:
    f.write("<!DOCTYPE html>")
    f.write("<html>")
    f.write("<head>")
    f.write("<title>Title of the document</title>")
    f.write("</head>")

for x,log in enumerate(namelist):
    print("log file : " + log)
    plotFile = os.path.splitext(log)[0] + ".png"
    parset = os.path.splitext(log)[0] + ".in"

    reLine = re.compile("mssink took ([0-9]*(\.[0-9]*)?)")

# This is the data for /scratch2 run
    with open(log) as fp:
        count = 0
        values = [];
        badCount = 0;
        minimum = 99
        maximum = 0
        for line in iter(fp.readline, ''):
            match = reLine.search(line)
            if match:
                count += 1
                secs = float("" + match.group(1))
                values.append(secs)
                if secs > 1.5:
                    badCount += 1
                if secs < minimum:
                    minimum = secs
                if secs > maximum:
                    maximum = secs

    N = len(values)
    mean = sum(values)/N
    bad = float(badCount)/N*100
    sdev = np.std(values)
    median = np.median(values)

    print("statistics of integration time for ", log, ":")
    print("integration time: min   : {0:.2f}".format(minimum))
    print("integration time: max   : {0:.2f}".format(maximum))
    print("integration time: mean  : {0:.2f}".format(mean))
    print("integration time: median: {0:.2f}".format(median))
    print("integration time: sdev  : {0:.2f}".format(sdev))

    with open(parset) as fp:

    #print("integration time: bad   : {0:.2f}%".format(bad))
    Row = namedtuple('Row',['Min','Max','Mean'])
    data1 = Row(minimum,maximum,median)
    pprinttable([data1],htmlFile)

    runningMean = np.convolve(values, np.ones((100,))/100, mode="valid")

#    Plotting
    timeMin = 0.0
    timeMax = 1.5*max(values)

    pl.figure(1)
    pl.subplot(211)
    plot = pl.plot(values, "b.", label="times")
    pl.axhline(sum(values)/N, color="red", label="average")
    pl.xlim(0, N)
    pl.ylim(timeMin, timeMax)
    title = "MSSink writing times per integration for log " +  log
    pl.title(title)
    pl.xlabel("integration cycle")
    pl.ylabel("time (s)")
    pl.legend(numpoints=1)

#pl.subplot(312)
#plot = pl.plot(runningMean, "r-", label="avg")
#pl.xlim(0, N)
#pl.ylim(0)
#pl.title("MSSink writing times average")
#pl.xlabel("integration")
#pl.ylabel("seconds")


    pl.subplot(212)
#pl.hist(values, 40, alpha=0.5)
    pl.hist(values, range=(timeMin, timeMax), bins=100)
    pl.xlim(timeMin, timeMax)
    interval = float(timeMax/10)

    pl.xticks(np.arange(timeMin, timeMax, interval))
    pl.title("Distribution")
    pl.xlabel("time (s)")
    pl.ylabel("count")

    pl.tight_layout()

# display the plot (must use interactive backend!)
#pl.show()

# save into file
    pl.savefig(plotFile)
    pl.close()
    print("plot file: " + plotFile)

with open (htmlFile,"a") as f:
    f.write("</html>")
