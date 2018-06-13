#!/usr/bin/env python

class StatsCollection:

    def __init__(self,options):

        self.beamlist = options.beamlist
        self.numfields = options.numfields
        self.beams = options.beams

        self.fieldBeamDict={}
        self.fieldBeamDict['Full']={}
        for f in range(self.numfields):
            label='F%02d'%f
            self.fieldBeamDict[label]={}
            for beam in self.beams:
                label='F%02d_B%02d'%(f,int(beam))
                self.fieldBeamDict[label]={}

    def addEntry(self,entry):
        if entry.beam >= 0:
            label='F%02d_B%02d'%(entry.field,entry.beam)
        elif entry.field >= 0:
            label='F%02d'%entry.field
        else:
            label='Full'

        self.fieldBeamDict[label][entry.label]=entry
        #print "Adding Entry: ",label,entry.label

    def plot(self,ax):
        ticks=[]
        labels=[]
        for fb in self.fieldBeamDict:
            #print fb,self.fieldBeamDict[fb]
            if len(self.fieldBeamDict[fb]) > 0:
                ticks.append(self.fieldBeamDict[fb].values()[0].ypos)
                labels.append(fb)
            for e in self.fieldBeamDict[fb]:
                self.fieldBeamDict[fb][e].plot(ax)
        ax.set_yticks(ticks)
        ax.set_yticklabels(labels,size='x-small')
