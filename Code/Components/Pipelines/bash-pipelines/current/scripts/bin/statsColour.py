#!/usr/bin/env python

import matplotlib.patches as mpatches
import matplotlib.lines as mlines
import matplotlib.pyplot as plt
import numpy as np

#labelTypes=['split', 'flag', 'Bandpass', 'apply', 'avg', 'contsub', 'contcube',
#                'cont_ccal', 'cont_selavy', 'contSC', 'spec_', 'linmos', 'selavy']
labelTypes=['split', 'flag', 'apply', 'avg', 
                'cont_ccal', 'contSC', 'contcube', 'contsub', 'spec_', 'linmos', 'selavy']
colourDict={'split':'C1',
            'flag':'C3',
            'apply':'C8',
            'average':'C6',
            'contsub':'C7',
            'calibrate':'C0',
            'contimage':'C2',
            'specimage':'C9',
            'linmos':'C4',
            'sourcefind':'C5'}

def getColour(label):
    
    if label.find('split')>=0 or label.find('copy')>=0:
        colour = colourDict['split']
    elif label.find('flag')>=0:
        colour = colourDict['flag']
# NOT DOING CALIBRATION JOBS, SO DON'T NEED THIS
#    elif label.find('Bandpass')>=0:
#        colour = 'C2'
    elif label.find('apply')>=0:
        colour = colourDict['apply']
    elif label.find('avg')>=0:
        colour = colourDict['average']
    elif label.find('linmos')>=0:
        colour = colourDict['linmos']
    elif label.find('cont')>=0:
        if label.find('contsub')>=0:
            if label.find('selavy')>=0:
                colour = colourDict['sourcefind']
            else:
                colour = colourDict['contsub']
        elif label.find('contcube')>=0:
            colour = colourDict['specimage']
        else:
            if label.find('ccal')>=0:
                colour = colourDict['calibrate']
            elif label.find('selavy')>=0:
                colour = colourDict['sourcefind']
            else:
                colour = colourDict['contimage']
    elif label.find('spec_')>=0:
        colour = colourDict['specimage']
    elif label.find('selavy')>=0:
        colour = colourDict['sourcefind']
    else:
        colour = 'k'

    return colour



def getDesc(label):
    
    if label.find('split')>=0:
        desc = 'Split/Copy'
    elif label.find('flag')>=0:
        desc = 'Flagging'
#    elif label.find('Bandpass')>=0:
#        desc = 'Bandpass'
    elif label.find('apply')>=0:
        desc = 'Apply Calibration'
    elif label.find('avg')>=0:
        desc = 'Averaging'
    elif label.find('cont')>=0:
        if label.find('contsub')>=0:
            desc = 'Continuum Subtraction'
        elif label.find('contcube')>=0:
            desc = 'Continuum Cube Imaging'
        else:
            if label.find('ccal')>=0:
                desc = 'Gains Calibration'
            elif label.find('selavy')>=0:
                desc = 'Source Finding'
            else:
                desc = 'Continuum Imaging'
    elif label.find('spec_')>=0:
        desc = 'Spectral Imaging'
    elif label.find('linmos')>=0:
        desc = 'Mosaicking'
    elif label.find('selavy')>=0:
        desc = 'Source Finding'
    else:
        desc = 'Other'

    return desc
        
def colourKey(loc="best"):

    patchlist=[]
    for lab in labelTypes:
        patchlist.append(mpatches.Patch(color=getColour(lab), label=getDesc(lab)))
                             
    plt.legend(handles=patchlist,loc=loc,fontsize='x-small')
    
def widthKey(loc='best'):
    
    ncores=np.array([1,3,10,30,100,300,1000,3000,472*20])

    for nc in ncores:
        plt.scatter([], [], c='k', s=nc, marker='s', label='%d cores'%nc)

    plt.legend(scatterpoints=1, loc=loc)
    
##    patchlist=[]
##    for nc in ncores:
##        yloc = ncores.
##        width = 0.8 * np.log10(nc+1) / np.log10(472.*20+1)
##        patchlist.append(mlines.Line2D([],[],linewidth=width,label='%d cores'%nc,color='k'))
##
##    plt.legend(handles=patchlist, loc=loc)
