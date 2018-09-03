#!/usr/bin/env python
#
# beamwiseCubeStats.py
#
# A python script to consolidate the individual beam cube-stats plots into one.
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

import casacore.images.image as im
import numpy as np
import os
import pylab as plt
from argparse import ArgumentParser

import cubestatsHelpers as cs

#########################

class BeamStats:
    def __init__(self, catalogue,size):
        self.cat=catalogue
        self.specSize = size

    def read(self):
        self.catGood = os.access(self.cat,os.F_OK)
        if self.catGood:
            self.maxval=np.zeros(self.specSize,dtype='f')
            self.minval=np.zeros(self.specSize,dtype='f')
            self.rms=np.zeros(self.specSize,dtype='f')
            self.madfm=np.zeros(self.specSize,dtype='f')
            fin=open(self.cat,'r')
            for line in fin:
                if line[0] != '#':
                    cols=line.split()
                    chan=int(line.split()[0])
                    self.maxval[chan] = float(line.split()[7])
                    self.minval[chan] = float(line.split()[6])
                    self.rms[chan] = float(line.split()[3])
                    self.madfm[chan] = float(line.split()[5])


    def plotNoise(self,freq):
        ymin,ymax=plt.ylim()
        if self.catGood:
            plt.plot(freq,self.rms,label='RMS')
            plt.plot(freq,self.madfm,label='scaled MADFM')
            ymax = self.rms[abs(self.rms)<1.e5].max()
            ymin = self.rms[abs(self.rms)<1.e5].min()
            ymax = np.max([ymax,self.madfm[abs(self.madfm)<1.e5].max()])
            ymin = np.min([ymin,self.madfm[abs(self.madfm)<1.e5].min()])
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
            plt.xlabel('Frequency [MHz]')
            plt.ylabel('Noise level [%s]'%unit)
        return ymin,ymax
    
    def plotMinMax(self,freq):
        ymin,ymax=plt.ylim()
        if self.catGood:
            plt.plot(freq,self.minval,label='Min')
            plt.plot(freq,self.maxval,label='Max')
            ymax = self.maxval[abs(self.maxval)<1.e5].max()
            ymin = self.minval[abs(self.minval)<1.e5].min()
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
            plt.xlabel('Frequency [MHz]')
            plt.ylabel('Min or Max flux level [%s]'%unit)
        return ymin,ymax


#########################

if __name__ == '__main__':

    parser = ArgumentParser(description="A python program to plot the statistics of all individual beam spectral cubes")
#    parser.add_argument("-c","--cube", dest="cube", type="string", default="", help="Input spectral cube or image [default: %(default)s]")
    parser.add_argument("-c","--cubeReference", dest="cube")
#                            help="The beam 0 cube name")

    args = parser.parse_args()

    # Identify the beam00 part of the cube name - this will be replaced by the
    # correct beam string.
    beamloc=args.cube.find('beam00')
    if beamloc < 0:
        print('Could not find "beam00" in the input cube - cannot use!')
        exit(1)

    # Define the output filenames, based on the cube name.
    # cubeTag is just the filename, without any extension (like .fits)
    # or leading path
    cubeNoFITS = args.cube
    if cubeNoFITS[-5:]=='.fits': cubeNoFITS = cubeNoFITS[:-5]
    cubeTag = os.path.basename(cubeNoFITS)
    cubeTagNoBeam = cubeTag.replace('.beam00','')
    cubeDir = os.path.dirname(cubeNoFITS)
    if cubeDir=='': cubeDir='.'
    catalogue='%s/cubeStats-%s.txt'%(cubeDir,cubeTag)

    cube=im(args.cube)
    coords=cube.coordinates()
    specSize = cs.findSpectralSize(shape,coords)
    freq = cs.getFreqAxis(cube)/1.e6

    beams=[]    
    for i in range(36):
        beamcat = catalogue.replace('beam00','beam%02d'%i)
        beam.append(BeamStats(beamcat,specSize))
        beam[i].read()

    fig=plt.figure(1,figsize=(12,3))
    for i in range(36):
        plt.subplot(12,3,i)
        ymin,ymax=beam[i].plotNoise(freq)
        if i==0 or plotymin>ymin: plotymin=ymin
        if i==0 or plotymax>ymax: plotymax=ymax
            
    for i in range(36):
        plt.subplot(12,3,i)
        plt.ylim(plotymin,plotymax)
        plt.text(0.02,0.9,'Beam %02d'%i,transform=ax.transAxes,horizontalalignment='left')
    plt.suptitle(cubeTagNoBeam)
    plt.savefig('beamNoise_%s.png'%cubeTagNoBeam)
    
    fig=plt.figure(2,figsize=(12,3))
    for i in range(36):
        plt.subplot(12,3,i)
        ymin,ymax=beam[i].plotMinMax(freq)
        if i==0 or plotymin>ymin: plotymin=ymin
        if i==0 or plotymax>ymax: plotymax=ymax
            
    for i in range(36):
        plt.subplot(12,3,i)
        plt.ylim(plotymin,plotymax)
        plt.text(0.02,0.9,'Beam %02d'%i,transform=ax.transAxes,horizontalalignment='left')
    plt.suptitle(cubeTagNoBeam)
    plt.savefig('beamMinMax%s.png'%cubeTagNoBeam)
    
