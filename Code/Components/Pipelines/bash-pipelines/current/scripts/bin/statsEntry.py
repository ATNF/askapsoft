#!/usr/bin/env python

import statsColour as sc
from datetime import datetime, timedelta
import numpy as np

class StatsEntry:

    def __init__(self,line, options):
        self.options = options
        self.isValid = False
        cols=line.split()
        self.label=cols[2]
        if self.label.find('worker') >= 0: return
        if self.isCal(): return
        if (len(cols) > 9 ) and (len(cols[9].split('T'))==2) and (len(cols[9].split('T')[0].split('-'))==3):
            self.isValid=True
            self.starttime=datetime.strptime(cols[9].split(',')[0],'%Y-%m-%dT%H:%M:%S')
            self.success = cols[3]
            self.ncores = int(cols[1])
            self.getLength(cols[4])
            self.getWidth()
            self.findFieldBeam()
            self.getYpos()
            self.colour = sc.getColour(self.label)

    def valid(self):
        return self.isValid
            
    def isCal(self):
        return ((self.label.find('BPCAL')>=0) or (self.label.find('Bandpass')>=0))

    def getLength(self, col):
        if self.success == 'OK':
            self.length = int(round(float(col)))
        else:
            self.length = 0

    def findFieldBeam(self):
        if self.isCal():
            self.field=-2
            if len(self.label.split('BPCALB'))>1:
                self.beam=self.label.split('BPCALB')[1].split('_')[0]
            else:
                self.beam=-1
        else:
            l=np.array(self.label.split('_'))
            if l[(np.array([a[0] for a in l])=='F')].size == 0:
                self.field,self.beam=-2,-2
            else:
                fb = l[(np.array([a[0] for a in l])=='F')][0]
                if fb == 'Full':
                    self.field,self.beam=-2,-2
                else:
                    if len(fb.split('B'))>1:
                        self.field,self.beam=fb.split('F')[1].split('B')
                    else:
                        self.field=fb.split('B')[0].split('F')[1]
                        self.beam=-1
        self.field=int(self.field)
        self.beam = int(self.beam)
        self.beamID = -1
        if (self.options.beams==self.beam).any():
            self.beamID = self.options.beams.tolist().index(self.beam)
        #print self.label,self.field,self.beam,self.beamID

    def getYpos(self):
        nf=self.options.numfields
        
        if self.beam >= 0:
            self.ypos = (self.field)*(self.options.beams.size+1) + self.beamID
        else:
            if self.beam == -1:
                self.ypos = (self.field)*(self.options.beams.size+1) + self.options.beams.size
            else:
                self.ypos = nf * (self.options.beams.size+1)
                
        self.ypos = float(self.ypos)

        offsets={'restored':-0.2,'residual':-0.1,'image':0.1,'contsub':0.2}
        if self.label.find('linmos')>=0:
            #print('Linmos job - %s'%self.label)
            for off in offsets:
                if self.label.find(off)>=0:
                    self.ypos += offsets[off]
                    #print "Offsetting %s by %s"%(self.label,offsets[off])

    def getWidth(self):
        self.width = 0.8 * np.log10(self.ncores+1) / np.log10(472.*20+1)

    def plot(self,ax):

        beg = self.starttime
        if self.success == 'OK':
        
            end = beg + +timedelta(seconds=self.length)
        
            if self.options.verbose:
                print('%s: yloc=%f, beg=%s, end-beg=%f, length=%s, col=%s'%
                        (self.label, self.ypos, beg.strftime('%T-%H%M'), (end-beg).seconds,
                            self.length, self.colour))

            ax.fill_between((beg,end),
                                self.ypos-self.width/2.,
                                self.ypos+self.width/2.,
                                color=self.colour)

        else:

            if self.options.verbose:
                print('%s: FAILED, yloc=%f, beg=%s, col=%s'%
                        (self.label, self.ypos, beg.strftime('%T-%H%M'), self.colour))
                
            ax.plot(beg, self.ypos + 0.5, 'x', color=self.colour)
