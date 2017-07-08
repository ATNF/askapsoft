#!/usr/bin/env python

import askap.analysis.evaluation

import matplotlib
matplotlib.use('Agg')
matplotlib.rcParams['font.family'] = 'serif'
matplotlib.rcParams['font.serif'] = ['Times', 'Palatino', 'New Century Schoolbook', 'Bookman', 'Computer Modern Roman']
#matplotlib.rcParams['text.usetex'] = True
import aplpy
import pylab as plt
from astropy.io import fits
import os
import numpy as np
from optparse import OptionParser

# Import ASKAP-specific packages
import askap.parset as parset
import askap.logging as logging
from askap.analysis.evaluation.readData import *

#############

if __name__ == '__main__':

    # Read the Parameter set - this is given with the -c command-line flag
    
    parser = OptionParser()
    parser.add_option("-c","--config", dest="inputfile", default="", help="Input parameter file [default: %default]")

    (options, args) = parser.parse_args()

    if(options.inputfile==''):
        inputPars = parset.ParameterSet()        
    elif(not os.path.exists(options.inputfile)):
        logging.warning("Config file %s does not exist!  Using default parameter values."%options.inputfile)
        inputPars = parset.ParameterSet()
    else:
        inputPars = parset.ParameterSet(options.inputfile).makeThumbnail

    # Read the parameters from the Parameter Set. These are the
    # user-configurable parameters that determine the data we want to
    # plot and various options.
    fitsim=inputPars.get_value('image','')
    weightsim=inputPars.get_value('weights','')
    zmin=inputPars.get_value('zmin',-10.)
    zmax=inputPars.get_value('zmax',40.)
    catalogue = inputPars.get_value('catalogue','')
    suffix=inputPars.get_value('imageSuffix','png')
    outdir=inputPars.get_value('outdir','')
    figtitle=inputPars.get_value('imageTitle','')
    figsizes=inputPars.get_value('imageSizes',[16])
    figsizenames = inputPars.get_value('imageSizeNames',['large'])
    showWeightsContours = inputPars.get_value('showWeightsContours',False)
    robust = inputPars.get_value('robust',True)
    weightCutoff = inputPars.get_value('weightscutoff',0.)

    # Error checking
    
    if len(figsizes) != len(figsizenames):
        raise IOError("figsizes and figsizenames must be the same length")
    
    if fitsim=='':
        raise IOError("No image defined")

    if not os.access(fitsim,os.F_OK):
        logging.error('Requested image %s not found'%fitsim)

    # Define the weights image - we use this for plotting contours and
    # determining the correct noise level.
    if weightsim != "" and not os.access(weightsim,os.F_OK):
        logging.warn('Weights image %s not found - no weights contours applied'%weightsim)

    # Get statistics for the image to determine greyscale levels.
    #  If there is a matching weights image, use that to
    #  avoid pixels that have zero weight.
    image=fits.getdata(fitsim)
    isgood=(np.ones(image.shape)>0)
    if weightsim!="" and os.access(weightsim,os.F_OK):
        weights=fits.getdata(weightsim)
        isgood=(weights>weightCutoff)
    isgood = isgood * ~np.isnan(image)
    if robust:
        median=np.median(image[isgood])
        madfm=np.median(abs(image[isgood]-median))
        stddev=madfm * 0.6744888
    else:
        stddev=np.std(image[isgood])
    print("Noise in image measured to be %f"%stddev)
    vmin=zmin * stddev
    vmax=zmax * stddev
    print("Greyscale range is from %f to %f"%(vmin,vmax))

    # Output name
    thumbim=os.path.basename(fitsim).replace('.fits','.%s'%suffix)
    if outdir == '':
        outdir=os.path.dirname(fitsim)
    if outdir != '':
        thumbim = '%s/%s'%(outdir,thumbim)

    # Text for labelling the colourbar
    if fitsim[:7]=='weights':
        colorbartext='Weight relative to peak'
    else:
        colorbartext=fits.getheader(fitsim)['BUNIT']

    # Can have more than one figure size, so we loop over the list
    # the user has provided.
    for i,size in enumerate(figsizes):

        filename=thumbim.replace('.%s'%suffix,'_%s.%s'%(figsizenames[i],suffix))
        print("Writing to file %s"%filename)

        # Load the figure
        gc=aplpy.FITSFigure(fitsim,figsize=(size,size))

        # If the figure being plotted is a weights image, we show both
        # the colour scale and the contours (for clarity). The
        # contours are labelled.
        # If it isn't a weights image, we show the image as a
        # greyscale and overplot the weights contours (if the
        # corresponding weights image exists).
        if fitsim[:7]=='weights':
            gc.show_colorscale(vmin=0.,vmax=1.)
            cs=plt.contour(image.squeeze(),levels=np.arange(10)/10.,colors='k')
            if size>10:
                fontsize=10
            else:
                fontsize=7
            plt.clabel(cs,fontsize=fontsize)
        else:
            #gc.show_colorscale(vmin=vmin,vmax=vmax)
            # get array via FITSIO and plot directly, to be robust against NaNs
            image=fits.getdata(fitsim)
            # reshape to remove degenerate axes
            newdim=[]
            for d in image.shape:
                if d > 1:
                    newdim.append(d)
            image2=image.reshape(newdim)
            plt.imshow(image2, vmin=vmin, vmax=vmax, cmap='Greys')
            if os.access(weightsim,os.F_OK) and showWeightsContours:
                cs=plt.contour(weights.squeeze(),levels=np.arange(0,10,2)/10.,colors='k',alpha=0.5)
                plt.clabel(cs,fontsize=10)

        # Formatting for the sky plot
        gc.tick_labels.set_xformat('hh:mm')
        gc.tick_labels.set_yformat('dd:mm')
        gc.add_grid()
        gc.grid.set_linewidth(0.5)
        gc.grid.set_alpha(0.5)
        plt.title(figtitle)
        # Add the colour bar to the side of the plot, showing the
        # range of units covered by the greyscale
        if fitsim[:7]=='weights':
            gc.add_colorbar()
            gc.colorbar.set_axis_label_text(colorbartext)
        else:
            cbar = plt.colorbar()
            cbar.set_label(colorbartext)
        #
        # If a catalogue has been provided, read it and plot ellipses
        # for each component.
        #  NOTE - No checking is done to see if the catalogue is the
        #  right format.
        # We use the readCat function in askap/analysis/evaluation/readData.py
        if not catalogue == '':
            sourceCat = readCat(catalogue,'Selavy')
            for id in sourceCat:
                source=sourceCat[id]
                print('%s %f %f %f %f %f'%(source.name,source.ra,source.dec,source.maj/3600.,source.min/3600.,source.pa))
                gc.show_ellipses(source.ra,source.dec,source.min/3600.,source.maj/3600.,source.pa,color='r')

        # Set the theme of the plot to something nice, then write out
        # to an image.
        gc.set_theme('publication')
        gc.save(filename)

