#!/usr/bin/env python
#
# beamwisePSFstats.py
#
# A python script to plot the individual beam cubes' PSF data
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

def getGoodCells(arr):
    med=np.median(arr)
    q75,q25=np.percentile(arr,[75,25])
    iqr=q75-q25
    issmall=(abs(arr)<1000.)
    isgood=(abs(arr-med) < 5.*iqr)
    return isgood*issmall

class PSFStats:
    def __init__(self, catalogue,size):
        self.cat=catalogue
        self.specSize = size

    def read(self):
        self.catGood = os.access(self.cat,os.F_OK)
        if self.catGood:
            self.bmaj=np.zeros(self.specSize,dtype='f')
            self.bmin=np.zeros(self.specSize,dtype='f')
            self.bpa=np.zeros(self.specSize,dtype='f')
            fin=open(self.cat,'r')
            for line in fin:
                if line[0] != '#':
                    cols=line.split()
                    chan=int(line.split()[0])
                    self.bmaj[chan] = float(line.split()[1])
                    self.bmin[chan] = float(line.split()[2])
                    self.bpa[chan] = float(line.split()[3])

    def sizeMinMax(self):
        ymin,ymax=0.,1.
        if self.catGood:
#            ymax = self.bmaj[getGoodCells(self.bmaj)].max()
#            ymin = self.bmaj[getGoodCells(self.bmaj)].min()
#            ymax = max(ymax,self.bmin[getGoodCells(self.bmin)].max())
#            ymin = min(ymin,self.bmin[getGoodCells(self.bmin)].max())
            ymax = self.bmaj.max()
            ymin = self.bmin.min()
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
        return ymin,ymax

    def paMinMax(self):
        ymin,ymax=0.,1.
        if self.catGood:
            ymax = self.bpa[getGoodCells(self.bpa)].max()
            ymin = self.bpa[getGoodCells(self.bpa)].min()
            width = ymax-ymin
            ymax = ymax + 0.1*width
            ymin = ymin - 0.1*width
        return ymin,ymax

    def plotSize(self,ax,freq):
        if self.catGood:
            ax.plot(freq,self.bmaj,label='BMAJ')
            ax.plot(freq,self.bmin,label='BMIN')
    
    def plotPA(self,ax,freq):
        if self.catGood:
            ax.plot(freq,np.unwrap(self.bpa),label='BPA',color='C2')


#########################

if __name__ == '__main__':

    parser = ArgumentParser(description="A python program to plot the PSF properties of all individual beam spectral cubes")
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
    catalogue='%s/beamlog.%s.txt'%(cubeDir,cubeTag)

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

    PAs=[]
    sizeYmin = sizeYmax = None
    paYmin = paYmax = None
    for i in range(36):
        beamcat = catalogue.replace('beam00','beam%02d'%i)
        PAs.append(PSFStats(beamcat,specSize))
        PAs[i].read()
        if PAs[i].catGood:
            ymin,ymax=PAs[i].sizeMinMax()
            if sizeYmin==None or sizeYmin>ymin: sizeYmin=ymin
            if sizeYmax==None or sizeYmax<ymax: sizeYmax=ymax
            ymin,ymax=PAs[i].paMinMax()
            if paYmin==None or paYmin>ymin: paYmin=ymin
            if paYmax==None or paYmax<ymax: paYmax=ymax


    fig, axs = plt.subplots(6,6, sharex=True, sharey=True, figsize = (12,6))
    fig.subplots_adjust(bottom=0.2, top=0.8, hspace=0.005, wspace=0.1)
    fig.text(0.5,0.001, 'Frequency [MHz]', ha='center')
    fig.text(0.01,0.5, 'PSF major & minor axis [arcsec]', ha='center', rotation='vertical')
    fig.text(0.99,0.5, 'PSF position angle [deg]', ha='center', rotation='vertical')
    fig.text(0.25,0.93,'BMAJ (left axis)',color='C0',ha='center')
    fig.text(0.50,0.93,'BMIN (left axis)',color='C1',ha='center')
    fig.text(0.75,0.93,'BPA (right axis)',color='C2',ha='center')
    axs = axs.ravel()
    for i in range(36):
        PAs[i].plotSize(axs[i],freq)
        axs[i].set_ylim(sizeYmin,sizeYmax)
        axs[i].text(freq[specSize/20],sizeYmin+0.8*(sizeYmax-sizeYmin),'Beam %02d'%i,ha='left', va='center',fontsize='x-small')
        ax2=axs[i].twinx()
        PAs[i].plotPA(ax2,freq)
        ax2.set_ylim(-180.,180.)
    plt.tight_layout(rect=[0.02,0.,1.,0.95])
    plt.suptitle(cubeTagNoBeam,y=0.98)
    plt.savefig('beamPSF_%s.png'%cubeTagNoBeam)
    
