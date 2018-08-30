#!/usr/bin/env python
#
# findCubeStatistics.py
#
# A python script to measure various statistics as a function of
# channel for a given spectral cube.
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
from mpi4py import MPI
import pylab as plt
from argparse import ArgumentParser

#############

def madfmToSigma(madfm):
    return madfm/0.6744888

def findSpectralIndex(coords):
    return coords.get_coordinate('spectral').get_image_axis()

def findSpectralSize(shape, coords):
    specIndex = findSpectralIndex(coords)
    return shape[specIndex]

def findSpatialSize(coords):

    for i in range(3):
        if coords.dict().has_key('direction%d'%i):
            label='direction%d'%i
    return coords.dict()[label]['_axes_sizes']

def getFreqAxis(cube):
    coords=cube.coordinates()
    shape=cube.shape()
    specDim=shape[findSpectralIndex(coords)]
    specCoo = coords.get_coordinate('spectral')
    freq = (np.arange(specDim)-specCoo.get_referencepixel())*specCoo.get_increment() + specCoo.get_referencevalue()
    return freq

class stat:
    def __init__(self,size,comm):
        self.rank = comm.Get_rank()
        self.nranks = comm.Get_size()
        self.size = size
        self.stat = np.zeros(self.size, dtype='f')
        self.fullStat = None
        self.finalStat = None
        if self.rank == 0:
            self.fullStat = np.empty([self.nranks,self.size],dtype='f')

    def assign(self,chan,statsDict,label):
        self.stat[chan] = statsDict[label]

    def gather(self,comm):
        comm.Gather(self.stat,self.fullStat)
        if self.rank == 0:
            self.finalStat = np.zeros(self.size)
            for i in range(self.size): self.finalStat[i] = self.fullStat[:,i].sum()


class statsCollection:
    def __init__(self,size,comm):
        self.rms = stat(size,comm)
        self.mean = stat(size,comm)
        self.median = stat(size,comm)
        self.madfm = stat(size,comm)
        self.maxval = stat(size,comm)
        self.minval = stat(size,comm)

    def assign(self, chan, stats):
        self.rms.assign(chan,stats,'rms')
        self.mean.assign(chan,stats,'mean')
        self.median.assign(chan,stats,'median')
        self.madfm.assign(chan,stats,'medabsdevmed')
        self.maxval.assign(chan,stats,'max')
        self.minval.assign(chan,stats,'min')

    def gather(self, comm):
        # Gather everything to the master
        self.rms.gather(comm)
        self.mean.gather(comm)
        self.median.gather(comm)
        self.madfm.gather(comm)
        self.maxval.gather(comm)
        self.minval.gather(comm)

    def getRMS(self): return self.rms.finalStat
    def getMean(self): return self.mean.finalStat
    def getMedian(self): return self.median.finalStat
    def getMadfm(self): return self.madfm.finalStat
    def getMaxval(self): return self.maxval.finalStat
    def getMinval(self): return self.minval.finalStat


#############
if __name__ == '__main__':

    parser = ArgumentParser(description="An MPI-distributed program to find the statistics of a spectral cube as a function of channel. The channels of the cube are distributed evenly across all available ranks, and their statistics are calculated independently, then sent to the rank-0 process for output as a catalogue and a graphical plot.")
#    parser.add_argument("-c","--cube", dest="cube", type="string", default="", help="Input spectral cube or image [default: %(default)s]")
    parser.add_argument("-c","--cube", dest="cube")
#                            help="Input spectral cube or image")

    args = parser.parse_args()

    if args.cube == '':
        print("Spectral cube not given - you need to use the -c option")
        exit(0)

    # Define the output filenames, based on the cube name.
    # cubeTag is just the filename, without any extension (like .fits)
    # or leading path
    cubeTag = os.path.basename(os.path.splitext(args.cube)[0])
    cubeDir = os.path.dirname(args.cube)
    if cubeDir=='': cubeDir='.'
    catalogue='%s/cubeStats-%s.txt'%(cubeDir,cubeTag)
    graph='%s/cubePlot-%s.png'%(cubeDir,cubeTag)
    
    # get the MPI rank
    comm = MPI.COMM_WORLD
    rank = comm.Get_rank()
    size = comm.Get_size()

    cube=im(args.cube)
    unit=cube.unit()
    scale=1
    if unit[:2]=='Jy':
        scale=1000.
        unit='m'+unit
    shape=cube.shape()
    blc=np.zeros_like(shape)
    trc=np.array(shape)-1
    coords=cube.coordinates()
    spInd = findSpectralIndex(coords)
    specSize = findSpectralSize(shape,coords)
    spatSize = findSpatialSize(coords)
    freq = getFreqAxis(cube)/1.e6

    stats = statsCollection(specSize,comm)
    
    for i in range(specSize):

        if i % size == rank:

            blc[spInd] = i
            trc[spInd] = i
            sub=cube.subimage(blc=blc.tolist(),trc=trc.tolist())
            subStats=sub.statistics()
            stats.assign(i,subStats)

    stats.gather(comm)
            
    if rank == 0:

        rms=stats.getRMS()*scale
        mean=stats.getMean()*scale
        median=stats.getMedian()*scale
        madfm=madfmToSigma(stats.getMadfm()*scale)
        minval=stats.getMinval()*scale
        maxval=stats.getMaxval()*scale
        
        fout=open(catalogue,'w')
        fout.write('#%7s %15s %10s %10s %10s %10s %10s %10s\n'%('Channel','Frequency','Mean','RMS','Median','MADFM','Min','Max'))
        fout.write('#%7s %15s %10s %10s %10s %10s %10s %10s\n'%(' ','MHz',unit,unit,unit,unit,unit,unit))
        for i in range(specSize):
            fout.write('%8d %15.6f %10.3f %10.3f %10.3f %10.3f %10.3f %10.3f\n'%(i, freq[i], mean[i], rms[i], median[i], madfm[i], minval[i], maxval[i]))
        fout.close()
        
        fig=plt.figure(1,figsize=(8,8))

        plt.subplot(211)
        plt.plot(freq,maxval,label='max')
        plt.plot(freq,minval,label='min')
        ymax = maxval[abs(maxval)<1.e5].max()
        ymin = minval[abs(minval)<1.e5].min()
        width = ymax-ymin
        ymax = ymax + 0.1*width
        ymin = ymin - 0.1*width
        plt.ylim(ymin,ymax)
        plt.xlabel('Frequency [MHz]')
        plt.ylabel('Flux value [%s]'%unit)
        plt.legend(loc='lower right')

        plt.subplot(212)
        plt.plot(freq,rms,label='RMS')
        plt.plot(freq,madfm,label='scaled MADFM')
        ymax = rms[abs(rms)<1.e5].max()
        ymin = rms[abs(rms)<1.e5].min()
        ymax = np.max([ymax,madfm[abs(madfm)<1.e5].max()])
        ymin = np.min([ymin,madfm[abs(madfm)<1.e5].min()])
        width = ymax-ymin
        ymax = ymax + 0.1*width
        ymin = ymin - 0.1*width
        plt.ylim(ymin,ymax)
        plt.xlabel('Frequency [MHz]')
        plt.ylabel('Flux value [%s]'%unit)
        plt.legend(loc='lower right')
        
        fig.savefig(graph)

