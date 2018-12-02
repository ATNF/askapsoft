#!/usr/bin/env python
# @file
#
# A class to handle a collection of stats entries (ie. usually
# corresponding to a single beam/field combination), complete with the
# plotting routines. 
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
