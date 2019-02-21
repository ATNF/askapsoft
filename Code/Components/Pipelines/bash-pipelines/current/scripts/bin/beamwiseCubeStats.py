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
from matplotlib.ticker import AutoMinorLocator,AutoLocator

import casacore.images.image as im
import numpy as np
import os
import pylab as plt
from matplotlib.ticker import MultipleLocator
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
            self.std=np.zeros(self.specSize,dtype='f')
            self.madfm=np.zeros(self.specSize,dtype='f')
            self.onepc=np.zeros(self.specSize,dtype='f')
            fin=open(self.cat,'r')
            for line in fin:
                if line[0] != '#':
                    cols=line.split()
                    chan=int(line.split()[0])
                    self.maxval[chan] = float(line.split()[8])
                    self.minval[chan] = float(line.split()[7])
                    self.std[chan] = float(line.split()[3])
                    self.madfm[chan] = cs.madfmToSigma(float(line.split()[5]))
                    self.onepc[chan] = float(line.split()[6])

    def noiseMinMax(self):
        ymin,ymax=0.,1.
        if self.catGood:
            ymax = self.std[cs.getGoodCells(self.std)].max()
            ymin = self.std[cs.getGoodCells(self.std)].min()
            ymax = max(ymax,self.madfm[cs.getGoodCells(self.madfm)].max())
            ymin = min(ymin,self.madfm[cs.getGoodCells(self.madfm)].max())
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
        return ymin,ymax

    def minMax(self):
        ymin,ymax=0.,1.
        if self.catGood:
            ymax = self.maxval[cs.getGoodCells(self.maxval)].max()
            ymin = self.minval[cs.getGoodCells(self.minval)].min()
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
        return ymin,ymax

    def plotNoise(self,ax,freq):
        if self.catGood:
            ax.plot(freq,self.std,label='Std. Dev.')
            ax.plot(freq,self.madfm,label='scaled MADFM')
    
    def plotMinMax(self,ax,freq):
        if self.catGood:
            ax.plot(freq,self.minval,label='Min')
            ax.plot(freq,self.maxval,label='Max')
            ax.plot(freq,self.onepc, label='1-percentile')


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

    # Find the first good image - it may be we haven't made a beam00 image...
    goodBeam=-1
    for i in range(36):
        name=args.cube.replace('beam00','beam%02d'%i)
        if os.access(name,os.F_OK):
            goodBeam=i
            break
    if goodBeam < 0:
        print('No good image found with name like %s. Exiting.'%args.cube)
        exit(1)
        
    cube=im(name)
    fluxunit=cube.unit()
    if fluxunit[:2]=='Jy':
        fluxunit='m'+fluxunit
    coords=cube.coordinates()
    specSize = cs.findSpectralSize(cube.shape(),coords)
    freq = cs.getFreqAxis(cube)/1.e6

    beams=[]
    noiseYmin = noiseYmax = None
    fullYmin = fullYmax = None
    for i in range(36):
        beamcat = catalogue.replace('beam00','beam%02d'%i)
        beams.append(BeamStats(beamcat,specSize))
        beams[i].read()
        if beams[i].catGood:
            ymin,ymax=beams[i].noiseMinMax()
            if noiseYmin==None or noiseYmin>ymin: noiseYmin=ymin
            if noiseYmax==None or noiseYmax<ymax: noiseYmax=ymax
            ymin,ymax=beams[i].minMax()
            if fullYmin==None or fullYmin>ymin: fullYmin=ymin
            if fullYmax==None or fullYmax<ymax: fullYmax=ymax

    if noiseYmin == None:
        print('No valid cubeStats catalogues found matching pattern %s'%catalogue)
        exit(1)
                
    width=noiseYmax-noiseYmin
    noiseYmin -= 0.1*width
    noiseYmax += 0.1*width
    width=fullYmax-fullYmin
    fullYmin -= 0.1*width
    fullYmax += 0.1*width

    fig, axs = plt.subplots(6,6, sharex=True, sharey=True, figsize = (12,6))
    fig.subplots_adjust(bottom=0.125, top=0.9, hspace=0.005, wspace=0.1)
    fig.text(0.5,0.005, 'Frequency [MHz]', ha='center')
    fig.text(0.01,0.5, 'Noise level [%s]'%fluxunit, va='center', rotation='vertical')
    fig.text(0.40,0.93,'Std. Dev.',color='C0', ha='center')
    fig.text(0.60,0.93,'scaled MADFM',color='C1', ha='center')
    axs = axs.ravel()
    for i in range(36):
        if beams[i].catGood:
            beams[i].plotNoise(axs[i],freq)
        axs[i].set_ylim(noiseYmin,noiseYmax)
        axs[i].text(freq[specSize/20],noiseYmin+0.9*(noiseYmax-noiseYmin),'Beam %02d'%i,ha='left',va='center',fontsize='x-small')
        #axs[i].yaxis.set_major_locator(AutoLocator(4))
        axs[i].yaxis.set_minor_locator(AutoMinorLocator(2))
    fig.tight_layout(rect=[0.02,0.,1.,0.95])
    fig.suptitle(cubeTagNoBeam)
    fig.savefig('beamNoise_%s.png'%cubeTagNoBeam)
    
    fig, axs = plt.subplots(6,6, sharex=True, sharey=True, figsize = (12,6))
    fig.subplots_adjust(bottom=0.2, top=0.8, hspace=0.005, wspace=0.1)
    fig.text(0.5,0.001, 'Frequency [MHz]', ha='center')
    fig.text(0.01,0.5, 'Min or Max flux level [%s]'%fluxunit, va='center', rotation='vertical')
    fig.text(0.25,0.93,'Min flux',color='C0', ha='center')
    fig.text(0.50,0.93,'1-percentile',color='C2', ha='center')
    fig.text(0.75,0.93,'Max flux',color='C1', ha='center')
    axs = axs.ravel()
    for i in range(36):
        beams[i].plotMinMax(axs[i],freq)
        axs[i].set_ylim(fullYmin,fullYmax)
        axs[i].text(freq[specSize/20],fullYmin+0.8*(fullYmax-fullYmin),'Beam %02d'%i,ha='left', va='center',fontsize='x-small')
        axs[i].yaxis.set_minor_locator(AutoMinorLocator(2))
    fig.tight_layout(rect=[0.02,0.,1.,0.95])
    fig.suptitle(cubeTagNoBeam)
    fig.savefig('beamMinMax_%s.png'%cubeTagNoBeam)
    
