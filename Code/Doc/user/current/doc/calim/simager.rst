simager
=======

This page provides details about *simager*, the spectral-line imaging
program. The purpose of this program is to provide a distributed way
of creating a large image cube, making use of a parallel processing
system.

This program is a prototype, designed to show a scalable solution to
distributed spectral-line processing. It is meant to be an interim
solution, available until a spectral-line imager within an improved
software framework is made available.

Running the program
-------------------

It can be run with the following command, where "config.in" is a file
containing the configuration parameters described in the next
section. ::
 
   $ <MPI wrapper> simager -c config.in

Parallel/Distributed Execution
------------------------------

The program is distributed and uses a master/worker pattern to
distribute and manage work. The master process (note that only one of
the processes is designated the master) is responsible for creating
and writing the output image cubes. It does no imaging itself. 

The worker processes get assigned individual channels by the master,
and these channels are imaged independently. Once all the imaging
is completed on a channel, the worker sends the various arrays back to
the master for writing to the cube.

Any number of processes can be allocated to an simager job. The
available channels are allocated to workers as the workers become
available, until all channels have been imaged.

On the Cray XC30 (*galaxy*) platform executing with the MPI wrapper
takes the form::

    $ aprun -n 4000 -N 20 simager -c config.in

The *-n* and *-N* parameters to the *aprun* application launcher
specify 4000 MPI processes will be used (3999 workers and one master)
and each node will host 20 MPI processes. This job then requires 200
compute nodes.

Configuration parameters
------------------------

The parameter set interface for *simager* differs a little from the
*cimager* interface. The interface to defining the image cube
properties is a bit more streamlined, without the requirement to
include the image name in the parameter name, so that many of them
start simply with **Simager.Images.**. The *simager*-specific
parameters are summarised in the following table.

Unlike *cimager*, *simager* does not have the advise capability
enabled, which would allow the user to leave out certain parameters
(such as direction, cellsize etc). The cost of filling these in by
reading the measurement set is likely to be much larger for *simager*,
so all parameters must be provided.

The different modules, for gridding, cleaning, restoring, have the
same interface, and are referenced in the same manner as for
*cimager* - that is, via **Simager.gridder.XXX**,
**Simager.solver.XXX**, **Simager.restore.XXX** and so forth.

Note, however, that on-the-fly application of calibration parameters
is not (yet) enabled (that is, there is no equivalent of the
**Cimager.calibrate=true** option).
Users are recommended to use :doc:`ccalapply` to apply the calibration
solution directly to the measurement set prior to imaging.

There are a few spectral-line-specific parameters that have been
introduced to *simager*.

* The user can define the rest frequency to be
  stored in the image cube metadata, either by giving a frequency or
  using the shortcut 'HI' (= 1420.405751786 MHz).
* When restoring, each channel is treated separately, so if
  **Simager.restore.beam=fit**, then each channel will have a
  different restoring beam. Since only a single beam can be recorded
  in the image, the user can select which channel is used as the
  reference, by using the **Simager.restore.beamReference**
  parameter. This can be a channel number (0-based), or one of
  'first', 'last' or 'mid'.
* To record the individual channel beams, simager will produce an ASCII text file
  listing the beam parameters for each channel. This is known as the
  "beam log". If the image cube name is "image.i.blah", then the beam
  log will be called "beamlog.image.i.blah.txt". The file has columns:
  index | major axis [arcsec] | minor axis [arcsec] | position angle [deg]
  Should the imaging of a channel fail for some reason, the beam for
  that channel will be recorded as having zero for all three
  parameters. This beam log is compatible with other askapsoft tasks,
  specfically the spectral extraction in Selavy (see
  :doc:`../analysis/extraction`). 

Here is an example of the start of a beam log::
  
  #Channel BMAJ[arcsec] BMIN[arcsec] BPA[deg]
  0 64.4269 59.2985 -70.8055
  1 64.4313 59.299 -70.8831
  2 64.4333 59.3018 -70.9345
  3 64.4338 59.2996 -70.9256
  4 64.4349 59.2982 -70.9108


+--------------------------+------------------+--------------+--------------------------------------------------------+
|**Parameter**             |**Type**          |**Default**   |**Description**                                         |
+==========================+==================+==============+========================================================+
|dataset                   |string or         |None          |Measurement set file name(s) to read. If the parameter  |
|                          |vector<string>    |              |is given as a vector of strings all measurement sets    |
|                          |                  |              |given by this vector are treated as being concatenated  |
|                          |                  |              |together (each worker will only get a single frequency  |
|                          |                  |              |channel at a time anyway).                              |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.name               |string            |None          |The base name of the image cubes to be produced. Only a |
|                          |                  |              |single name is accepted, and it must (as for *cimager*) |
|                          |                  |              |start with 'image'.                                     |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.direction          |vector<string>    |None          |Direction to the centre of the required image (or       |
|                          |                  |              |tangent point for facets). This vector should contain a |
|                          |                  |              |3-element direction quantity containing right ascension,|
|                          |                  |              |declination and epoch, e.g. [12h30m00.00, -45.00.00.00, |
|                          |                  |              |J2000]. Note that a casa style of declination delimiters|
|                          |                  |              |(dots rather than colons) is essential. Only *J2000*    |
|                          |                  |              |directions are currently supported. Note also that this |
|                          |                  |              |must be provided, as the advise capability is not       |
|                          |                  |              |enabled.                                                |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.shape              |vector<int>       |None          |Shape in pixels of the image cube's spatial axes.       |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.cellsize           |vector<string>    |None          |A two-element vector of quantity strings, indicating the|
|                          |                  |              |size of pixels in the spatial axes, e.g. [6.0arcsec,    |
|                          |                  |              |6.0arcsec]                                              |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.polarisation       |vector<string>    |["I"]         |Polarisation planes to be produced for the image (should|
|                          |                  |              |have at least one). Polarisation conversion is done     |
|                          |                  |              |on-the-fly, so the output polarisation frame may differ |
|                          |                  |              |from that of the dataset. An exception is thrown if     |
|                          |                  |              |there is insufficient information to obtain the         |
|                          |                  |              |requested polarisation (e.g. there are no cross-pols and|
|                          |                  |              |full stokes cube is requested). Note, ASKAPsoft uses the|
|                          |                  |              |correct definition of stokes parameters, i.e. I=XX+YY,  |
|                          |                  |              |which is different from casa and miriad (which imply    |
|                          |                  |              |I=(XX+YY)/2).The code parsing the value of this         |
|                          |                  |              |parameter is quite flexible and allows many ways to     |
|                          |                  |              |define stokes axis, e.g. [“XX YY”] or [“XX”,”YY”] or    |
|                          |                  |              |“XX,YY” are all acceptable                              |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|Images.restFrequency      |string            |None          |A string indicating the rest frequency to be written to |
|                          |                  |              |the image cube header (for the restored, model and      |
|                          |                  |              |residual cubes only). The string can be a quantity      |
|                          |                  |              |string (e.g. 1234.567MHz) or the special string 'HI',   |
|                          |                  |              |which resovles to 1420.405751786 MHz. If not given, no  |
|                          |                  |              |rest frequency is written to the cubes.                 |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|restore                   |bool              |false         |If true, the image will be restored (by convolving with |
|                          |                  |              |the given 2D gaussian), in the same manner as for       |
|                          |                  |              |:doc:`cimager`. The restoration is done separately for  |
|                          |                  |              |each channel.                                           |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|restore.beam              |vector<string>    |None          |Either a single word 'fit' or a quantity string         |
|                          |                  |              |describing the shape of the clean beam (to convolve the |
|                          |                  |              |model image with). If quantity is given it must have    |
|                          |                  |              |exactly 3 elements, e.g. [30arcsec, 10arcsec,           |
|                          |                  |              |40deg]. Otherwise an exception is thrown. This parameter|
|                          |                  |              |is only used if *restore* is set to True. If            |
|                          |                  |              |*restore.beam=fit*, the code will fit a 2D gaussian to  |
|                          |                  |              |the PSF image and use the results of this fit. In this  |
|                          |                  |              |case, each channel with have an independently-fitted    |
|                          |                  |              |beam.                                                   |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|restore.beamReference     |string            |mid           |The channel to use as the reference for the beam - this |
|                          |                  |              |channel's beam is written to the cube header. Values can|
|                          |                  |              |be an integer indicating the channel number (0-based),  |
|                          |                  |              |or one of 'mid', 'first', or 'last'.                    |
|                          |                  |              |                                                        |
+--------------------------+------------------+--------------+--------------------------------------------------------+
|restore.beam.cutoff       |double            |0.05          |Cutoff for the support search prior to beam fitting, as |
|                          |                  |              |a fraction of the PSF peak. This parameter is only used |
|                          |                  |              |if *restore.beam=fit*. The code does fitting on a       |
|                          |                  |              |limited support (to speed things up and to avoid        |
|                          |                  |              |sidelobes influencing the fit). The extent of this      |
|                          |                  |              |support is controlled by this parameter representing the|
|                          |                  |              |level of the PSF which should be included into          |
|                          |                  |              |support. This value should be above the first sidelobe  |
|                          |                  |              |level for meaningful results.                           |
+--------------------------+------------------+--------------+--------------------------------------------------------+
                    
Example parset
--------------

.. code-block:: bash

   Simager.dataset                                = spectralLineData.ms
   #
   Simager.Images.name                            = image.i.cube
   Simager.Images.shape                           = [2048,2048]
   Simager.Images.cellsize                        = [10arcsec,10arcsec]
   Simager.Images.direction                       = [17h44m25.4506, -51.44.43.791, J2000]
   Simager.Images.restFrequency                   = HI
   #
   Simager.gridder.snapshotimaging                = true
   Simager.gridder.snapshotimaging.wtolerance     = 2600
   Simager.gridder                                = WProject
   Simager.gridder.WProject.wmax                  = 2600
   Simager.gridder.WProject.nwplanes              = 99
   Simager.gridder.WProject.oversample            = 4
   Simager.gridder.WProject.diameter              = 12m
   Simager.gridder.WProject.blockage              = 2m
   Simager.gridder.WProject.maxfeeds              = 36
   Simager.gridder.WProject.maxsupport            = 512
   Simager.gridder.WProject.variablesupport       = true
   Simager.gridder.WProject.offsetsupport         = true
   Simager.gridder.WProject.frequencydependent    = true
   #
   Simager.solver                                 = Clean
   Simager.solver.Clean.algorithm                 = Basisfunction
   Simager.solver.Clean.niter                     = 500
   Simager.solver.Clean.gain                      = 0.3
   Simager.solver.Clean.scales                    = [0,3,10]
   Simager.solver.Clean.verbose                   = False
   Simager.solver.Clean.tolerance                 = 0.01
   Simager.solver.Clean.weightcutoff              = zero
   Simager.solver.Clean.weightcutoff.clean        = false
   Simager.solver.Clean.psfwidth                  = 512
   Simager.solver.Clean.logevery                  = 50
   Simager.threshold.minorcycle                   = [30%, 15mJy]
   Simager.threshold.majorcycle                   = 20mJy
   Simager.ncycles                                = 3
   Simager.Images.writeAtMajorCycle               = false
   #
   Simager.restore                                = true
   Simager.restore.beam                           = fit
   Simager.restore.beamReference                  = first
   #
   Simager.preconditioner.Names                   = [Wiener, GaussianTaper]
   Simager.preconditioner.GaussianTaper           = [50arcsec, 50arcsec, 0deg]
   Simager.preconditioner.Wiener.robustness       = 0.25
