#!/usr/bin/env python
#
# Post processing all IOR log files in the current directory.
#
# Copyright: CSIRO 2017
# Author: Paulus Lahur <paulus.lahur@csiro.au>
#----------------------------------------------------------------------|

import os
import argparse
import csv
import re
import numpy as np
import matplotlib
# Use non-interactive backend to prevent figure from popping up
# Note: Agg = Anti Grain Geometry
matplotlib.use('Agg')
import matplotlib.pyplot as pl

# Parameter name (must match the name within log file)
# TODO: Set this using input
#param_name = 'stripe size'
#param_name = 'stripe count'
#param_name = 'xfersize'
param_name = 'filename'

# Directory name of the log files
dir = 'log'

# TODO: Check if the directory exists

file_list = [f for f in os.listdir(dir) 
        if os.path.isfile(os.path.join(dir, f))]
#print file_list

header_list = (param_name, 'write max', 'write min', 'write mean', 'write sdev',
        'read max', 'read min', 'read mean', 'read sdev')
output = list()
output.append(header_list)

for f in file_list:
    print f
    file_name = dir + '/' + f
    #print file_name
    file = open(file_name, 'r')
    
    output_line = list()
    param_found = False
    summary_found = False
    write_performance_found = False
    read_performance_found = False
    error_found = False

    for line in file.readlines():
        # Get the parameter value (x value)
        line = line.strip()
        if (line.find(param_name) >= 0):
            #print 'found'
            value_string = line[line.find('=')+1:]
            value_string = value_string.strip()
            #print value_string
            param_found = True
            output_line.append(value_string)
        elif (line.find('WARNING') >= 0):
            error_found = True
            break
        elif (line.find('ERROR') >= 0):
            error_found = True
            break
            
        # If summary section is found (near the end of the file)
        if (summary_found):
            # Get the write and read performance
            if (line.find('write') >= 0):
                values = line.split()
                write_performance_found = True
                for value in values[1:5]:
                    output_line.append(value)
                #print values
            elif (line.find('read') >= 0):
                values = line.split()
                read_performance_found = True
                for value in values[1:5]:
                    output_line.append(value)
                #print values
        else:
            if (line.find('Summary of all tests') >= 0):
                summary_found = True
                #print 'Summary found'
    # next line

    if (not error_found):
        if (not param_found):
            print 'ERROR: cannot find parameter: ' + param_name
            SystemExit
        if (not summary_found):
            print 'ERROR: cannot find summary section'
            SystemExit
        if (not write_performance_found):
            print 'ERROR: cannot find write performance'
            SystemExit
        if (not read_performance_found):
            print 'ERROR: cannot find read performance'
            SystemExit

        #print output_line
        output.append(list(output_line))

    del output_line[:]

    file.close()
# next log file

# Output data in CSV format
# parameter name, write max, write min, write mean, write sdev, 
#            read max, read min, read mean, read sdev
#print output
output_file = open('output.csv', 'wb')
wr = csv.writer(output_file, quoting=csv.QUOTE_NONE)
for output_line in output:
    wr.writerow(output_line)
