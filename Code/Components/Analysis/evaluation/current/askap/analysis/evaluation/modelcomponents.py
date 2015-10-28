#!/usr/bin/env python

## @file
#  A file containing definitions of model components

from math import *
from utils import *

## @namespace modelcomponents
#  Model components used by the analysis & cross-matching code, that
#  will be referenced by the evaluation plots.

class SelavyObject:
    # This class is now meant to be equivalent to the CASDA component catalogue that Selavy creates
    def __init__(self,line):
        self.line = line
        if(line[0]!='#'):
            cols = line.split()
            self.type = 'Selavy'
            self.islandID = cols[0]
            self.id = cols[1]
            self.name = cols[2]
            self.raStr = cols[3]
            self.decStr = cols[4]
            self.ra = float(cols[5])
            self.dec = float(cols[6])
            self.raErr = float(cols[7])
            self.decErr = float(cols[8])
            self.freq = float(cols[9])
            self.Fpeak = float(cols[10])
            self.FpeakErr = float(cols[11])
            self.Fint = float(cols[12])
            self.FintErr = float(cols[13])
            self.maj = float(cols[14])
            self.min = float(cols[15])
            self.pa = float(cols[16])
            self.majErr = float(cols[17])
            self.minErr = float(cols[18])
            self.paErr = float(cols[19])
            self.majDECONV = float(cols[20])
            self.minDECONV = float(cols[21])
            self.paDECONV = float(cols[22])
            self.chisqFIT = float(cols[23])
            self.rmsFIT = float(cols[24])
            self.alpha = float(cols[25])
            self.beta = float(cols[26])
            self.RMSimage = float(cols[27])
            self.flag1 = int(cols[28])
            self.flag2 = int(cols[29])
            self.flag3 = int(cols[30])
            self.flag4 = int(cols[31])
#            print "Read component %s with flux %8.6f"%(self.id,self.Fint)

    def flux(self):
        return self.Fint

    def peak(self):
        return self.Fpeak

class FullStokesS3SEXObject:
    def __init__(self,line):
        self.line = line
        if line[0]!='#' :
            cols=line.split()
            self.type = 'FullStokesS3SEX'
            self.componentNum = int(cols[0])
            self.clusterID = int(cols[1])
            self.galaxyNum = int(cols[2])
            self.SFtype = int(cols[3])
            self.AGNtype = int(cols[4])
            self.structure = int(cols[5])
            self.ra = float(cols[6])
            self.dec = float(cols[7])
            self.distance = float(cols[8])
            self.redshift = float(cols[9])
            self.pa = float(cols[10])
            self.maj = float(cols[11])
            self.min = float(cols[12])
            self.I151 = float(cols[13])
            self.I610 = float(cols[14])
            self.Iref = float(cols[15])
            self.I1400 = log10(self.Iref)
            self.Qref = float(cols[16])
            self.Uref = float(cols[17])
            self.Pref = float(cols[18])
            self.Pfrac = float(cols[19])
            self.I4860 = float(cols[20])
            self.I18000 = float(cols[21])
            self.cosVA = float(cols[22])
            self.RM = float(cols[23])
            self.RMflag = float(cols[24])

    def flux(self):
        return self.Iref

    def peak(self):
        return self.flux()* pi * self.maj * self.min / (4.*log(2))

class ContinuumObject:
    def __init__(self,line):
        self.line = line
        if(line[0]!='#'):
            cols=line.split()
            self.type = 'Continuum'
            self.ra = posToDec(cols[0])
            self.dec = posToDec(cols[1])
            self.flux0 = float(cols[2])
            self.alpha = float(cols[3])
            self.beta = float(cols[4])
            self.maj = float(cols[5])
            self.min = float(cols[6])
            self.pa = float(cols[7])
            self.id='%s_%s'%(cols[0],cols[1])
            
    def flux(self):
        return self.flux0

    def peak(self):
        return self.flux()* pi * self.maj * self.min / (4.*log(2))

class ContinuumIDObject:
    def __init__(self,line):
        self.line = line
        if(line[0]!='#'):
            cols=line.split()
            self.type = 'ContinuumID'
            self.id = cols[0]
            self.ra = posToDec(cols[1])
            self.dec = posToDec(cols[2])
            self.flux0 = float(cols[3])
            self.alpha = float(cols[4])
            self.beta = float(cols[5])
            self.maj = float(cols[6])
            self.min = float(cols[7])
            self.pa = float(cols[8])
            
    def flux(self):
        return self.flux0

    def peak(self):
        return self.flux()* pi * self.maj * self.min / (4.*log(2))


class SUMSSObject:
    def __init__(self,line):
        self.line = line
        if(line[0]!='#'):
            cols=line.split()
            self.type = 'SUMSS'
            self.ra=15.*(float(cols[0])+float(cols[1])/60.+float(cols[2])/3600.)
            self.dec=abs(float(cols[3]))+float(cols[4])/60.+float(cols[5])/3600.
            if cols[3][0]=='-':
                self.dec = -1. * self.dec
            #self.id = cols[0]+":"+cols[1]+":"+cols[2]+"_"+cols[3]+":"+cols[4]+":"+cols[5]
            self.id = "J"+cols[0]+cols[1]+cols[3]+cols[4]
            self.raErr = float(cols[6])
            self.decErr = float(cols[7])
            self.Fpeak = float(cols[8])
            self.FpeakErr = float(cols[9])
            self.Fint = float(cols[10])
            self.FintErr = float(cols[11])
            self.maj = float(cols[12])
            self.min = float(cols[13])
            self.pa = float(cols[14])
            self.majDECONV = float(cols[15])
            self.minDECONV = float(cols[16])
            if cols[17] == '---':
                self.paDECONV = 0.
            else:
                self.paDECONV = float(cols[17])
            self.mosaicName = cols[18]
            self.numMosaics = int(cols[19])
            self.xpos = float(cols[20])
            self.ypos = float(cols[21])

    def flux(self):
        return self.Fint

    def peak(self):
        return self.Fpeak

def tofloat(str):
    if str.strip()=='':
        return 0.
    else:
        return float(str)
    
def toint(str):
    if str.strip()=='':
        return 0
    else:
        return int(str)
    
class NVSSObject:

    def __init__(self,line):
        self.line = line
        if(line[0]!='#'):
            self.type = 'NVSS'
            self.radius=tofloat(line[0:9])
            self.xoff = tofloat(line[9:19])
            self.yoff = tofloat(line[19:29])
            self.recno = toint(line[30:38])
            self.field = line[38:46]
            self.fieldxpos = tofloat(line[47:54])
            self.fieldypos = tofloat(line[55:62])
            self.name = line[63:77].strip()
            self.id = self.name
            self.rastring = line[78:89].replace(' ',':')
            self.ra = posToDec(self.rastring)*15.
            self.raerr = tofloat(line[90:95])
            self.decstring = line[96:107].replace(' ',':')
            self.dec = posToDec(self.decstring)
            self.decerr = tofloat(line[108:112])
            self.S1400 = tofloat(line[113:121])
            self.S1400err = tofloat(line[122:129])
            self.majorAxisLimit = line[130]
            self.maj = tofloat(line[132:137])
            self.majErr = tofloat(line[138:142])
            self.minorAxisLimit = line[143]
            self.min = tofloat(line[145:150])
            self.minErr = tofloat(line[151:155])
            self.pa =tofloat(line[156:161])
            self.paErr =  tofloat(line[162:166])
            self.flagResidual = line[167:169]
            self.residualFlux = toint(line[170:174])
            self.polFlux = tofloat(line[175:181])
            self.polPA = tofloat(line[182:187])
            self.polFluxErr = tofloat(line[188:193])
            self.polPAerr = tofloat(line[194:198])

    def flux(self):
        return self.S1400

    def peak(self):
        self.flux()* pi * self.maj * self.min / (4.*log(2))
    
class Match:
    def __init__(self,line,srcCat,refCat):
        cols=line.split()
        self.src=srcCat[cols[1]]
        self.ref=refCat[cols[2]]
        self.sep=float(cols[3])
        self.type=int(cols[0])
        self.setOffsets()

    def setOffsets(self,flagSphericalPos=True):
        self.dx=(self.src.ra-self.ref.ra)
        self.dy=(self.src.dec-self.ref.dec)
        if(flagSphericalPos):
            self.dx = self.dx * cos(0.5*(self.src.dec+self.ref.dec)*pi/180.)
