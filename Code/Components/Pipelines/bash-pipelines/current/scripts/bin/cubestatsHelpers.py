#!/usr/bin/env python
#
# cubestatsHelpers.py
#
# Python functions and classes to help with the measurement and handling of cube statistics
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
#############

import numpy as np

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

def getGoodCells(arr):
    med=np.median(arr)
    q75,q25=np.percentile(arr,[75,25])
    iqr=q75-q25
    issmall=(abs(arr)<1.e5)
    isgood=(abs(arr-med) < 5.*iqr)
    return isgood*issmall



##############

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
            for i in range(self.size):
                self.finalStat[i] = self.fullStat[:,i].sum()

    def __getitem__(self,i):
        return self.finalStat[i]


class statsCollection:
    def __init__(self,freq,comm,useCasacoreStats):
        self.freq = freq
        self.useCasa = useCasacoreStats
        self.scaleFactor = 1.
        self.units = 'Jy/beam'
        size = freq.size
        self.rms = stat(size,comm)
        self.std = stat(size,comm)
        self.mean = stat(size,comm)
        self.onepc = stat(size,comm)
        self.median = stat(size,comm)
        self.madfm = stat(size,comm)
        self.maxval = stat(size,comm)
        self.minval = stat(size,comm)

    def calculate(self, cube, chan, blc, trc):
        if self.useCasa:
            sub=cube.subimage(blc=blc,trc=trc)
            subStats=sub.statistics()
            arr = sub.getdata()
            msk = ~sub.getmask()
            subStats['onepc'] = np.percentile(arr[msk],1)
        else:
            arr = cube.getdata(blc=blc,trc=trc)
            msk = ~cube.getmask(blc=blc,trc=trc)
            percentiles = np.percentile(arr[msk],[50,1])
            print percentiles
            print percentiles[0]
            print percentiles[1]
            subStats={}
            subStats['median']=percentiles[0]
            subStats['medabsdevmed'] = np.median(abs(arr[msk]-med))
            subStats['onepc'] = percentiles[1]
            subStats['mean'] = arr[msk].mean()
            subStats['sigma'] = arr[msk].std()
            subStats['rms'] = np.sqrt(np.mean(arr[msk]**2))
            subStats['min'] = arr[msk].min()
            subStats['max'] = arr[msk].max()
            madfm = np.median(abs(arr[msk]-med))
                    
        self.assign(chan,subStats)
        
    def assign(self, chan, stats):
        self.rms.assign(chan,stats,'rms')
        self.std.assign(chan,stats,'sigma')
        self.mean.assign(chan,stats,'mean')
        self.onepc.assign(chan,stats,'onepc')
        self.median.assign(chan,stats,'median')
        self.madfm.assign(chan,stats,'medabsdevmed')
        self.maxval.assign(chan,stats,'max')
        self.minval.assign(chan,stats,'min')

    def gather(self, comm):
        # Gather everything to the master
        self.rms.gather(comm)
        self.std.gather(comm)
        self.mean.gather(comm)
        self.onepc.gather(comm)
        self.median.gather(comm)
        self.madfm.gather(comm)
        self.maxval.gather(comm)
        self.minval.gather(comm)

    def getRMS(self): return self.rms.finalStat
    def getStd(self): return self.std.finalStat
    def getMean(self): return self.mean.finalStat
    def getOnepc(self): return self.onepc.finalStat
    def getMedian(self): return self.median.finalStat
    def getMadfm(self): return self.madfm.finalStat
    def getMaxval(self): return self.maxval.finalStat
    def getMinval(self): return self.minval.finalStat

    def setScaleFactor(self,scalefactor):
        self.scaleFactor = scalefactor

    def setUnits(self, unit):
        self.unit = unit
        
    def scale(self):
        self.rms.finalStat = self.rms.finalStat * self.scaleFactor
        self.std.finalStat = self.std.finalStat * self.scaleFactor
        self.mean.finalStat = self.mean.finalStat * self.scaleFactor
        self.onepc.finalStat = self.onepc.finalStat * self.scaleFactor
        self.median.finalStat = self.median.finalStat * self.scaleFactor
        self.madfm.finalStat = self.madfm.finalStat * self.scaleFactor
        self.maxval.finalStat = self.maxval.finalStat * self.scaleFactor
        self.minval.finalStat = self.minval.finalStat * self.scaleFactor
        
    def write(self,catalogue):
        fout=open(catalogue,'w')
        fout.write('#%7s %15s %10s %10s %10s %10s %10s %10s %10s\n'%('Channel','Frequency','Mean','Std','Median','MADFM','1%ile','Min','Max'))
        fout.write('#%7s %15s %10s %10s %10s %10s %10s %10s %10s\n'%(' ','MHz',self.unit,self.unit,self.unit,self.unit,self.unit,self.unit,self.unit))
        for i in range(self.freq.size):
            fout.write('%8d %15.6f %10.3f %10.3f %10.3f %10.3f %10.3f %10.3f %10.3f\n'%(i, self.freq[i], self.mean[i], self.std[i], self.median[i], self.madfm[i], self.onepc[i], self.minval[i], self.maxval[i]))
        fout.close()
            
#    def read(catalogue):
#        if self.rank == 0:
            
