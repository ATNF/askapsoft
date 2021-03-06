ASKAPsoft Release Notes
=======================

This page summarises the key changes in each tagged release of
ASKAPsoft. This replicates the CHANGES file that is included in the
code base.

0.24.2 (TBC)
------------

This is planned as a patch release, and the following items have been
fixed:

Pipelines:

 * Changes to the distribution of ranks within the imaging job, to
   smooth out the memory usage a little.
 * Use the bptool module to identify antennas to flag based on the
   bandpass calibration table. Triggered by the new parameter
   DO_PREFLAG_SCIENCE.
 * Checkfiles will now indicate the time-window where appropriate, to
   better distinguish work that has been done on datasets.
 * Checks for spectral-line parameters are now only done when
   DO_SPECTRAL_PROCESSING is set to true as well as DO_SPECTRAL_IMAGING. 
 * Time-split data will now no longer be regenerated if the merged
   dataset is present.
 * New or changed pipeline default parameters:

   - DO_PREFLAG_SCIENCE (true)
   - GRIDDER_SHARECF and GRIDDER_SPECTRAL_SHARECF, to allow
     performance enhancements to be realised in the pipeline.
   - TILENCHAN_AV, TILENCHAN_SL, TILE_NCHAN_SCIENCE & TILE_NCHAN_1934
     all now default to 1.
   - SELAVY_NSUBY is now 6
   - ARCHIVE_SPECTRAL_MS is now false
   - MULTI_JOB_SELFCAL is now false

Processing:

 * A fix to Selavy to allow computation of spectral indices from
   extracted spectra - this was failing due to uninitialised memory.
 * There are numerous performance enhancements to imaging and
   linear-mosaicking, resulting in faster processing times.
 * The linear-mosaicking will now correct higher-order Taylor-term
   images for primary beam variation with frequency, through the use
   of the removebeam parameter.
 * The WProject gridder will now discard data above the wmax limit,
   rather than throwing an exception.
 * The WProject gridder now has the option of using a static cache of
   convolution functions, rather than re-generating for every instance
   of a gridder.


0.24.1 (8 May 2019)
-------------------

A patch release, focusing only on fixes for pipeline issues:

 * Default values for GRIDDER_WMAX_NO_SNAPSHOT and
   GRIDDER_SPECTRAL_WMAX_NO_SNAPSHOT have been changed to 35000
 * For the CASDA upload script, certain evaluation files have had the
   pipeline timestamp added to their filename
 * A few bugs were fixed:
   
   - The list of MSs given to the upload script were erroneously the
     final time-window MS (when that mode is used).
   - The continuum validation script was mistakenly making all files
     group-writeable (rather than just the copied validation directory).
   - The logic for checking the FREQ_FRAME parameter was incorrect and
     should now do the right thing.

The user documentation has also been updated, with a missing table
added back in.


0.24.0 (16 April 2019)
----------------------

A major release, with an improved framework for the pipeline scripts
to handle large datasets efficiently, along with several performance
improvements to the processing software.

Processing software:

 * Spectral imaging:

   - There is a change to the behaviour of the Channels keyword - now
     the syntax is [number,startchannel], where the channels are those
     in the input measurement set.
   - Introduction of more frequency frames. You can now specify one of
     three reference frames topocentric (default) barycentric and the
     local kinematic standard of rest. The new keyword is:
     Cimager.freqframe=topo | bary | lsrk
   - You can now specify the output channels via their frequencies if
     you choose. This is a very new feature but simple to test. Note
     that, as before, there is no interpolation in this mode. It is
     simply a “nearest” neighbour scheme. The syntax is:
     Cimager.Frequencies=[number,start,width], with the start and
     width parameters are in Hz.
   - The spectral line imager should be more robust to missing
     channels either via flagging or frequency conversion.
   - The restoring beam was often not being written to the header of
     spectral cubes, particularly when more than one writer was used.
   - The beamlog (the list of restoring beams per channel) for cubes
     was not being written correctly. It now will always be written,
     and will have every channel present, even those with no good data
     (their beams will be zero-sized).

 * The bandpass calibration had reverted to the issue solved in
   0.22.0, where a failed fit to a single channel/beam would crash the
   job. This is now robust against such failures again.
 * The mssplit tool has had a memory overflow fixed, so that
   bucketsizes larger than 4GB can be used.
 * The linmos-mpi task would fail with an error when the first image
   being processed was not the largest.
   
Pipelines:

 * The pre-imaging tasks can now run more efficiently by dividing the
   dataset into time segments and processing each segment
   separately. The length of the time segment is configurable. The
   time-splitting applies to the application of bandpass solutions,
   flagging, averaging, and, for the spectral data,
   continuum-subtraction.
 * Additional continuum-imaging parameters are made
   selcal-loop-dependent: CLEAN_ALGORITHM, CLEAN_MINORCYCLE_NITER,
   CLEAN_GAIN, CLEAN_PSFWIDTH, CLEAN_SCALES, and
   CLEAN_THRESHOLD_MINORCYCLE. The last two parameters can have
   vectors for individual loops, and so this necessitates a new
   format, whereindividual loops are separated by ' ; '.
 * Overall control over the spectral processing is now provided by
   DO_SPECTRAL_PROCESSING. This defaults to false, meaning only the
   continuum processing will be done. Turning this to true will result
   in application of the selfcal gains, continuum subtraction,
   spectral imaging and image-based continuum subtraction being done -
   each of these are turned on by default.
 * Elevation-based flagging for the science observation is able to be
   configured through the pipeline parameters.
 * There are parameters to specify a given range of times to be used
   from the bandpass calibration observation - useful if you wish to
   use part of a long observation.
 * The arguments to the bandpass smoothing tools can be provided as a
   text string instead of the specific pipeline parameters - this will
   allow continued development of these tools without needing to keep
   the pipeline up-to-date with possible parameter inputs.
 * The new spectral imaging features (see above) are exposed through
   pipeline parameters FREQ_FRAME_SL and OUTPUT_CHANNELS_SL. The
   DO_BARY parameter has been deprecated.
 * The following bugs have been fixed

   - The continuum subtraction selavy jobs were using the wrong
     nsubx/nsuby parameters.
   - The pipeline scripts now check for the correct loading of the
     askapsoft & askappipeline modules, and exit if these are not
     loaded correctly.
   - Wildcards used in identifying polarisation data products are now
     correctly applied.

 * There are new parameters used in the pipeline (with their defaults): 

   - DO_SPLIT_TIMEWISE=true
   - DO_SPLIT_TIMEWISE_SERIAL=true
   - SPLIT_INTERVAL_MINUTES=60
   - MULTI_JOB_SELFCAL=true
   - FAT_NODE_CONT_IMG=true
   - TILENCHAN_AV=18
   - SPLIT_TIME_START_1934=""
   - SPLIT_TIME_END_1934=""
   - BANDPASS_SMOOTH_ARG_STRING=""
   - BANDPASS_SMOOTH_F54=""
   - ELEVATION_FLAG_SCIENCE_LOW=""
   - ELEVATION_FLAG_SCIENCE_HIGH=""
   - OUTPUT_CHANNELS_SL=""
   - FREQ_FRAME_SL=bary
   - DO_SPECTRAL_PROCESSING=true

 * There are new default values for some pipeline parameters:

   - JOB_TIME_DEFAULT="24:00:00"
   - CPUS_PER_CORE_CONT_IMAGING=6
   - FAT_NODE_CONT_IMG=true
   - NUM_PIXELS_CONT=6144
   - CELLSIZE_CONT=2
   - NCHAN_PER_CORE=12
   - NCHAN_PER_CORE_CONTCUBE=3
   - NCHAN_PER_CORE_SL=64
   - NUM_SPECTRAL_WRITERS=""
   - NUM_SPECTRAL_WRITERS_CONTCUBE=""
   - GRIDDER_WMAX_NO_SNAPSHOT=30000
   - GRIDDER_NWPLANES_NO_SNAPSHOT=257
   - CLEAN_MINORCYCLE_NITER="[400,800]"
   - CLEAN_GAIN=0.2
   - CLEAN_SCALES="[0,3,10]"
   - CLEAN_THRESHOLD_MINORCYCLE="[30%, 0.5mJy, 0.03mJy]"
   - CLEAN_NUM_MAJORCYCLES="[5,10]"
   - CLEAN_THRESHOLD_MAJORCYCLE="0.035mJy"
   - SELFCAL_NUM_LOOPS=1
   - SELFCAL_INTERVAL=[200,200]
   - NUM_PIXELS_CONTCUBE=4096
   - CPUS_PER_CORE_CONTCUBE_IMAGING=8
   - CLEAN_CONTCUBE_MINORCYCLE_NITER=600
   - CLEAN_CONTCUBE_GAIN=0.2
   - CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE="[40%, 0.5mJy, 0.05mJy]"
   - CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE=0.06mJy
   - CLEAN_CONTCUBE_NUM_MAJORCYCLES=3
   - TILENCHAN_SL=18
   - DO_APPLY_CAL_SL=true
   - DO_CONT_SUB_SL=true
   - DO_SPECTRAL_IMAGING=true
   - DO_SPECTRAL_IMSUB=true
   - NUM_PIXELS_SPECTRAL=1024
   - CELLSIZE_SPECTRAL=8
   - SPECTRAL_IMAGE_MAXUV=2000
   - PRECONDITIONER_SPECTRAL_GAUSS_TAPER="[20arcsec, 20arcsec, 0deg]"
   - GRIDDER_SPECTRAL_SNAPSHOT_IMAGING=false
   - GRIDDER_SPECTRAL_WMAX_NO_SNAPSHOT=30000
   - GRIDDER_SPECTRAL_NWPLANES=257
   - CLEAN_SPECTRAL_MINORCYCLE_NITER=800
   - CLEAN_SPECTRAL_GAIN=0.2
   - CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE="[45%, 3.5mJy, 0.5mJy]"
   - CLEAN_SPECTRAL_THRESHOLD_MAJORCYCLE=0.5mJy
   - CLEAN_SPECTRAL_NUM_MAJORCYCLES=3


0.23.3 (22 February 2019)
-------------------------

A further patch release, with a number of small pipeline fixes, along
with several fixes to the processing software.

Processing:

 * The imager would produce slightly different residual and restored
   images when different values of nchanpercore were used. This was
   due to the final gridding cycle not being synchronised
   correctly. This has been fixed and the images are now indepenent of
   nchanpercore.
 * The tree reduction used by imager has been improved to have a
   smaller memory footprint across the cores.
 * The selavy component fitting is improved in the way negative
   components are handled. Unless negative components are explicitly
   accepted, if a fit results in one or more components being negative
   then that fit will be rejected. 
 * The primary beam used by linmos now has a FWHM scaling by 1.09
   lambda/D, which should be more accurate.
 * The FITSImage interface (in Code/Base/accessors) will now report a
   human-readable error message (rather than a number code) when an
   error occurs.

Pipelines:

 * CASDA uploads again include catalogues (which were left out due to
   fixes in 0.23.1).
 * There are new parameters ``CIMAGER_MAXUV`` and
   ``CCALIBRATOR_MAXUV`` that allow the imposition of an upper limit
   to the uv values in the continuum imaging/self-calibration.
 * Parsets for the imager were erroneously getting a
   "Cimager.Channels" selection that included the %w wildcard. This
   will no longer happen (unless cimager is used).
 * The default python module is now always loaded at the start of
   slurm scripts, to avoid python conflicts due to a user's particular
   environment.
 * There are stronger checks on the number of cores allocated to
   spectral-line imaging, ensuring that the number of channels must be
   an exact multiple of the nchanpercore.
 * The scaling on the beam-wise noise plots has been fixed, so that
   the scaled MADFM should be closer to the standard deviation in the
   absence of signal.
 * Cube stats are now also generated for continuum-cube residual
   images.
 * Several scripts have been tidied up with the aim of avoiding
   spurious errors (validationScience, for instance).
 * The ASKAPsoft version was being left off FITS headers. This now
   reflects the version string from the askapsoft module.

0.23.2 (2 February 2019)
------------------------

A patch release, fixing an issue with imager and a couple of minor pipeline issues:

 * The imager in spectral-imaging mode was not respecting the clean
   thresholds correctly. This could lead to over-cleaning, and the
   insertion of spurious clean components at noise peaks (particularly
   in continuum-subtracted spectral data).
 * A change has been made to the module setup, avoiding "module swap"
   in favour of "module unload / module load" - this addresses an
   occasional issue seen where the module environment can get
   corrupted by the swap command.
 * A fix has been made to the flagging parsets, solving a problem
   where the autocorrelation flagging and the time-range flagging were
   assigned to the same rule. If both were used, the time range was
   only flagged in the autocorrelations. They now appear as separate
   rules and so will be independent.

   
0.23.1 (22 January 2019)
------------------------

A patch release, addressing a few bugs in both processing software and pipeline scripts

Pipelines:

 * Changes have been made to the scripts to make them robust in
   handling field names that contain spaces. This has also made them
   more robust to being run in a directory with a path that contains
   spaces.
 * An update has been made at Pawsey to the module used for the
   continuum validation task, and consequently a minor change has been
   made to the continuum sourcefinding script.

Processing:

 * Enhancements have been made to the continuum-subtraction task
   ccontsubtract to speed it up - initial tests indicate a speed-up of
   6-8x depending on platform. 
 * The Selavy fitting algorithm now defaults to including a test on
   the size of the fitted Gaussians. This will prevent spuriously
   large fits from making it through to the catalogue, which has had
   detrimental effects in the calibration & continuum-subtraction.
 * A fix was made to the imager, solving a problem where the
   spectral-imaging option merged the first channel of its allocation
   without checking the frequency.

Additionally, the user documentation has updated instructions about
how best to set the modules on galaxy so that everything runs
smoothly (see :doc:`../platform/processing`).


0.23.0 (10 December 2018)
-------------------------

A major release, addressing a number of issues with the processing software and the pipeline scripts.

Pipelines:

 * When multiple raw MSs are provided for a given beam (split up by
   frequency range), the pipeline is capable of recognising this,
   merging (after any necessary splitting), and handling all required
   metadata appropriately. The functionality should be the same no
   matter the structure of the raw data.
 * The selfcal job allocation (for the sbatch call) has been altered
   to request a number of nodes, rather than cores +
   cores-per-node. This should provide more predictable allocations.
 * The weights cutoff parameter given to Selavy is now fully
   consistent with the linmos cutoff.
 * Fixed a bug that meant the raw data was overwritten when
   calibration was applied, even when KEEP_RAW_AV_MS=true.
 * The TELESCOP keyword is now added to the FITS headers.
 * A bug was fixed that was preventing the full-resolution MSs being
   included in the CASDA upload.
 * New parameters SPECTRAL_IMAGE_MAXUV and SPECTRAL_IMAGE_MINUV that
   allow control over the UV distances passed to the spectral imager.
 * Various improvements to the gatherStats job, so that it will still
   run after the killAll script has been called, and that looks for
   the pipeline-errors directory before trying to use it.
 * Making the cubeStats script more robust against failures of a
   single process (so that it doesn't hang but instead carries on as
   best it can).


Processing:

 * Imaging:
   
  - Fix a coordinate shift that was seen in spectral imaging, due to a
    different direction being provided by the advise functionality. 

 * Calibration:
   
  - Efficiency improvements to ccalapply to help speed it up

 * Utilities:
   
  - Adjustment of the maximum cache size in mssplit to avoid
    out-of-memory issues
  - Trimming down of the pointing table in MSs produced by msconcat,
    so that very large tables do not result. 

 * Selavy:
   
  - The restoring beam is now written into the component maps.
  - A significant change to the handling of the initial estimates for
    the Gaussian fits, making it more robust and avoiding downstream
    WCS errors that were hampering the analysis.
  - Minor catalogue fixes for component & HI catalogues
  - Segfaults in selfcal (3145)

0.22.2 (02 October 2018)
------------------------
Minor change to pipeline scripts:
 * nChan is now set to CHAN_RANGE_SCIENCE/1934 parameter instead of reading it from the
   raw measurement sets. Fixes the bug arising when working on subset channel 
   range in the measurement sets.

0.22.1 (25 September 2018)
--------------------------
A patch release to 0.22 to fix a couple of bugs:
 * Fixed issue with missing reading of visibilities causing zeros after calibration.
 * Added multi-row processing mode for Amplitude and StokesV flaggers.

0.22.0 (20 September 2018)
--------------------------

This release sees a number of changes & improvements to both the processing software and the pipeline scripts.

Pipelines:

 * There are new diagnostic plots produced, particularly for the spectral & continuum cubes. There is a python script run immediately following the imaging to calculate a range of statistics on a channel-by-channel basis, and this data is plotted on a per-image basis, as well as an overview plot showing the statistics for each beam at once.
 * The ability to specify the number of cores used for the continuum imaging has been improved, to make it more flexible and work better with slurm.
 * The behaviour of source-finding in the selfcal has changed. We now fit the full set of Gaussian parameters, and require contiguous pixels for the islands. 
 * Several bugs were fixed:
   
   - Some FITS files were not having their header keywords updated correctly. This has now been fixed and streamlined.
   - The CASDA upload script was erroneously including multiple versions of the taylor.1 images, due to a bug introduced in 0.21.0. It was also dropping a few .fits suffixes in certain situations.
   - The cmodel-based continuum subtraction script had a bug with an undefined local variable that occured with particular parameter settings.s
   - The clean-model-based self-calibration script was getting the model image name wrong for Taylor-term images.
     
 * There are a number of changes to the default parameters:
   
   - The DO_MAKE_THUMBNAILS option is now true by default.
   - There is a new DO_VALIDATION_SCIENCE (true by default) to run the cube validation.
   - The snapshot imaging has been turned off by default, as this has
     proved to be more reliable in terms of image quality. Along with
     this, the number of w-planes has a default value that changes
     with the snapshot option: snapshot imaging has 99, non-snapshot
     imaging has 599.
   - The number of channels in the MS tile is exposed as a parameter for the bandpass & science datasets, taking the same default value as previously (54).
   - The "solver.Clean.solutiontype" parameter is exposed as a pipeline parameter for all imaging jobs.
   - The SB_JIRA_ISSUE is replaced by JIRA_ANNOTATION_PROJECT for schedblock annotations, although this functionality is currently only available to the askapops user.

Applications:

 * Selavy:
   - The Selavy HI catalogue now has better defined default values, and the NaN values that were appearing have been fixed (through use of masked arrays when fitting to the moment-0 map).
   - Selavy was previously occasionally dropping components from the catalogue through the application of acceptance criteria. This is now optional, and off by default.
   - Selavy was failing to calculate spectral indices in certain cases - this is now fixed.
   - The deconvolved sizes in the Selavy components catalogue are now calculated with better floating-point arithmetic, to avoid rare cases of NaNs.
   - The column widths for the VOTable catalogues are more tightly controlled, in line with the CASDA software.
 * Imaging:
   - Primary beam factory (used by linmos) able to handle elliptical Gaussian beams. Not fully implemented within linmos yet.
   - A new gridder AltWProjectVisGridder that allows the dumping of uvgrids to casa images.
   - Caching in the spectral-line imaging of imager is now done by channel.
   - The spectral-line imager will now correctly write the beam log when using multiple writers.
   - An issue with the beam fitting failing for very elongated beams has been remedied.
 * Casda upload:
   - The casdaupload utility was leaving out the path to measurement sets when making the XML interface file, even when the absolute path option was being requested. This is now fixed and all artifacts will have their absolute path used in this mode.
   - Similarly, checksums for the thumbnail images were not being created by casdaupload. This has been remedied.
 * Other:
   - The FITS accessor interface now better handles missing header keywords - if they are not present it now logs a warning but doesn't exit.
   - Ccalapply has improved handling of flags, allowing write access.
   - Improvements to the efficiency of mssplit and msmerge.
   - The user documentation has a detailed tutorial on MS(MFS) imaging.


0.21.2 (31 July 2018)
---------------------

A further patch release for the pipeline, fixing a few issues that
have been seen on the Galaxy platform.

 * The previous fix for the OUTPUT directory is now included correctly
   in the release.
 * The fix for imagetype parameter in Selavy parsets generated by the
   pipeline has been extended to the continuum-subtraction jobs.
 * The bandpass validation log is copied to the diagnostics directory,
   as it includes useful information about the state of the dataset.
 * Errors involving 'find' (from the 'rejuvenate' function) are no
   longer reported in the slurm output when the file in question does
   not exist.
 * When aoflagger is used for the flagging, the slurm script ensures
   that the correct programming environment (PrgEnv module) is loaded
   prior to loading the aoflagger module.
 * The continuum cube imaging can now use more than one channel per
   core. This is accessed via the new parameter
   NCHAN_PER_CORE_CONTCUBE.
 * Added casa, aoflagger and bptool version reporting to the image
   headers and the copy of the config file, to enhance the
   reproducibility of the pipeline processing.
   

0.21.1 (17 July 2018)
---------------------

A patch release fixing minor issues with the 0.21.0 version of the
processing pipeline scripts. Only the scripts and the documentation
are changed.

Fixes to the pipeline:

 * The bandpass validation script will now find the correct files when
   an OUTPUT directory is used.
 * Similarly, the statsPlotter script is now more robust against the
   use of the OUTPUT option.
 * Parallel processing enabled for the larger ccalapply jobs.
 * The Channels selection parameter for continuum imaging can be left
   out when NUM_CPUS_CONTIMG_SCI is provided, with a new parameter
   CHANNEL_SELECTION_CONTIMG_SCI available to specify a selection.
 * The snapshot imaging option is turned back on by default for all
   imaging with the pipeline, following further testing & feedback
   from commissioning & operations teams.
 * There is better specification of the imagetype parameter in the
   Selavy parsets - there were issues when imagetype=casa was
   used. 



0.21.0 (6 July 2018)
---------------------

A large release containing a number of updates to the pipeline scripts
and to various aspects of the processing tools.

Pipeline updates:

 * Ability to use AOflagger instead of cflag.
 * Ability to use the continuum cubes to measure spectral indices of
   continuum components (using Selavy).
 * Fixing a bug where the CleanModel option of continuum-subtraction
   was using the wrong image name.
 * Allow self-calibration to use the clean model image as the model
   for calibration (in the manner of continuum-subtraction).
 * Improvement of the continuum subtraction Selavy parameterisations,
   to better model the continuum components. The Selavy parsets are
   now consistent with those used for the continuum cataloguing.
 * Collation of pipeline jobs that failed, for analysis by ASKAP
   Operations, to help identify pipeline or platform issues.
 * Use of an alternative bandpass smoothing task -
   smooth_bandpass.py (instead of plot_caltable.py).
 * Use of an additional bandpass validation script to produce summary
   diagnostic plots for the bandpass solutions.
 * Fixed a bug where the bandpass table name was not set correctly
   when the the DO_FIND_BANDPASS switch was turned off.
 * Addition of the spectral measurement sets, the
   continuum-subtraction models/catalogues, and the spectral cube beam
   logs to the list of artefacts to be sent to CASDA upon pipeline
   completion.
 * Added more robustness to the pipeline scripts to allow them to run
   on other systems, allowing the specification of the module
   directory and flexibility for running on non-Lustre filesystems.
 * Changes to some default parameters. Here are the parameters that
   have changed, with their new values (note that the WMAX and
   MAXSUPPORT gridding parameters now also adapt their default values
   according to whether snapshot imaging is turned on or off):

.. code-block:: bash

  # Image type
  IMAGETYPE_CONT=fits
  IMAGETYPE_CONTCUBE=fits
  IMAGETYPE_SPECTRAL=fits
  # Bandpass calibration
  DO_APPLY_BANDPASS_1934=true
  BANDPASS_SMOOTH_FIT=1
  BANDPASS_SMOOTH_THRESHOLD=1.0
  # Continuum imaging
  NUM_TAYLOR_TERMS=2
  CLEAN_MINORCYCLE_NITER=2000
  CLEAN_PSFWIDTH=256
  CLEAN_THRESHOLD_MINORCYCLE="[20%, 1.8mJy, 0.03mJy]"
  CLEAN_NUM_MAJORCYCLES="[5,10,10]"
  CLEAN_THRESHOLD_MAJORCYCLE="0.03mJy"
  SELFCAL_INTERVAL="[1800,1800,200]"
  GRIDDER_SNAPSHOT_IMAGING=false
  GRIDDER_WMAX_SNAPSHOT=2600
  GRIDDER_MAXSUPPORT_SNAPSHOT=512
  GRIDDER_WMAX_NO_SNAPSHOT=26000
  GRIDDER_MAXSUPPORT_NO_SNAPSHOT=1024
  # Continuum cube imaging
  CLEAN_CONTCUBE_ALGORITHM=BasisfunctionMFS
  CLEAN_CONTCUBE_PSFWIDTH=256
  CLEAN_CONTCUBE_MINORCYCLE_NITER=2000
  CLEAN_CONTCUBE_THRESHOLD_MINORCYCLE="[40%, 12.6mJy, 0.5mJy]"
  CLEAN_CONTCUBE_THRESHOLD_MAJORCYCLE=0.5mJy
  # Spectral imaging
  NCHAN_PER_CORE_SL=9
  NUM_SPECTRAL_WRITERS=16
  ALT_IMAGER_SINGLE_FILE=true
  PRECONDITIONER_LIST_SPECTRAL="[Wiener,GaussianTaper]"
  PRECONDITIONER_SPECTRAL_GAUSS_TAPER="[30arcsec, 30arcsec, 0deg]"
  PRECONDITIONER_SPECTRAL_WIENER_ROBUSTNESS=0.5
  CLEAN_SPECTRAL_ALGORITHM=BasisfunctionMFS
  CLEAN_SPECTRAL_PSFWIDTH=256
  CLEAN_SPECTRAL_SCALES="[0,3,10,30]"
  CLEAN_SPECTRAL_THRESHOLD_MINORCYCLE="[50%, 30mJy, 3.5mJy]"
  CLEAN_SPECTRAL_MINORCYCLE_NITER=2000
  GRIDDER_SPECTRAL_SNAPSHOT_IMAGING=false
  GRIDDER_SPECTRAL_WMAX_SNAPSHOT=2600
  GRIDDER_SPECTRAL_MAXSUPPORT_SNAPSHOT=512
  GRIDDER_SPECTRAL_WMAX_NO_SNAPSHOT=26000
  GRIDDER_SPECTRAL_MAXSUPPORT_NO_SNAPSHOT=1024
  # Spectral source-finding
  SELAVY_SPEC_OPTIMISE_MASK=false
  SELAVY_SPEC_VARIABLE_THRESHOLD=true
  SELAVY_SPEC_SNR_CUT=8

Processing tasks:

 * An MPI barrier has been added to the spectral imager to prevent
   race conditions in the writing.
 * Better handling of cases in the bandpass calibration that were
   previously (prior to 0.20.3) causing it to fail with SVD conversion
   errors.
 * Selavy will now report the best component fit (assuming it
   converges in the fitting), regardless of the chi-squared. If poor,
   a new flag will be set.
 * If the fit fails to converge, Selavy can reduce the number of
   Gaussians being fit to try to get a good fit.
 * A bug in Selavy was fixed to allow the curvature-map method of
   identifying components to better take into account the weights
   image associated with the image being searched.
 * A further bug in Selavy (the extraction code) was fixed to allow
   its use on images without spectral or Stokes axes.
 * The SNR image produced by Selavy now has a blank string for the
   pixel units.
 * The implementation of the variable threshold calculations in Selavy
   have been streamlined, to improve the memory usage particularly for
   large spectral cubes. There is also control over the imagetype for
   the images written as part of this algorithm.
 * The memory handling within linmos-mpi has been improved to reduce
   its footprint, making it better able to mosaic large spectral
   cubes. 

Manager & ingest:

 * Improvements to the CP manager.
 * UVW calculations fixed in the course of testing new fringe rotator modes.

ASKAPsoft environment:

 * Incorporation of python-casacore in the cpapps build (used to
   create the askapsoft module at Pawsey). 

Documentation:

 * Added a chapter to the user documentation on how to combine multiple
   epochs for spectral line data. 
 * Added a chapter to the user documentation explaining the best way
   to do MS/MFS deconvolution in askapsoft
 * Added a page to the user documentation listing the release notes
   for each release.
   

0.20.3 (2 April 2018)
---------------------

A patch release fixing a couple of calibrator issues:

 * The 0.20 updates to the calibrator to allow interaction with the
   calibration data service had prevented ccalibrator from writing
   more than one row to the output calibration table. This fix ensures
   the table that gets written has all the information when solving
   for time-dependent gains.
 * The bandpass calibrator would very occasionally fail with an error
   along the lines of "ERROR: SVD decomposition failed to
   converge". This will now only trigger a WARN in the log file, but
   will not abort the program. Work is still being done to properly
   flag channels that suffer this.

And a couple of pipeline issues have been fixed:

 * The beams that are processed by the pipeline are now limited by the
   number of beams in the bandpass calibrator scheduling block (in the
   same way that the science SB is used to limit the number of beams).
 * Minor issues with copying the continuum validation results have
   been resolved.

Additionally, casacore (in 3rdParty) is now built with the python
bindings, so that libcasa_python will be available.


0.20.2 (27 March 2018)
----------------------

A patch release that fixes a few bugs in the build to do with missing directories:

 * Modified several build configurations so that missing directories
   do not make the build fail. Missing directories can be present as a
   result of a bug in our SVN to BitBucket sync which ignores empty
   directories (even if there is a .gitxxxx file in it). Subsequently,
   cloning the git repo causes these directories to be missing which
   can cause a failed build for some packages. In these cases, the
   build script has been changed to create the missing directories if
   they are missing.
 * Note there are no application code or documentation changes for
   this release.

0.20.1 (08 March 2018)
----------------------

A patch release that fixes a few bugs in the pipeline:

 * Adds better robustness to the USE_CLI=false option, for use when
   the databases at MRO are unavailable.
 * A scripting error in the self-calibration script (for the Cmodel
   option).
 * Fixes to the defineArtifacts script, to better handle FITS
   extensions.
 * When the image-based continuum-subtraction option is run, the
   spectral source-finding job will now search the continuum-subtracted
   cube. The spectral source-finding will also handle sub-bands
   correctly. 
 * There have also been fixes to ensure the continuum-subtracted
   cubes are created in appropriate FITS format and mosaicked
   correctly.
 * Copying of continuum validation files to the archive directory has
   been updated to reflect an improved directory structure.

It also makes a few minor changes to the processing software:

 * The Wiener preconditioner will now report in the log the amount by
   which the point-source sensitivity is expected to increase over the
   theoretical naturally-weighted level.
 * The casdaupload utility can now produce an XML file with absolute
   paths to data products, leaving them in place - rather than copying
   all data products to the upload directory. This is compatible with
   behaviour introduced in CASDA-1.10.
 * Ccalapply has a new parameter than can restrict the sizes of chunks
   presented in single iterations, using new options for the
   TableDataSource classes.
 * The component catalogue produced by Selavy had a minor error in the
   calculation of the error on the integrated flux (where the minor
   axis should have been used, the major axis was used instead).
 * Fixed issues with cmodel functional tests, relating to using the
   correct catalogue columns.
 * Fixed a failing scimath unit test.
 * The ingest pipeline now can apply phase gradients in parallel. 
   

0.20.0 (09 February 2018)
-------------------------

This release sees the first version of the Calibration Data Service
(CDS) and Sky Model Service (SMS) in deployable form. These components
are intended to run independently of the ASKAPsoft pipelines. At
first, they will require some configuration and data
initialisation. Testing and feedback will then drive further
development.

The CDS provides an interface to a database containing calibration
parameters. The SMS allows access to the Global Sky Model data,
primarily for the purpose of constructing local sky models.

Other changes in this release include:

Pipelines:
 * Corrected the use of the $ACES environment variable when running
   the continuum validation script, so that pecularities of the local
   environment are appropriately dealt with. 
 * Some corrections in pipeline scripts regarding FITS mode processing:

   * Ensures the continuum linmos image is copied at the field-level
     mosaicking job.  
   * Ensures the spectral-line selavy job uses the correct file
     extensions.  
   * Ensures the imcontsub job converts the contsub cube to fits at
     the end if we are working in FITS mode.
   * Updates the naming of the contsub cube to ensure consistency
     (removing .fits from the middle of it).
     
 * Improve copying of spectral weights images when running linmos to
   avoid ambiguities and prevent unnecessary files. 
 * Added a parameter, DO_SOURCE_FINDING_FIELD_MOSAICS, to turn off
   source finding for individual fields and rely on the source finding
   for the final mosaic instead. This prevents unnecessary source
   finding jobs being launched. 
 * Selavy source finding jobs now have scheduling block ID (SBID)
   passed in parsets. 
 * The casdaupload utility can now handle cubelets (as well as spectra
   & moment-maps). These are included by the casda script in the
   pipeline.  
 * TIME selection options in flagging are now exposed in pipeline
   scripts via TIME_FLAG_SCIENCE, TIME_FLAG_SCIENCE_AV and
   TIME_FLAG_1934. It is up to the user to provide suitable values.
 * Pipelines allow processing of scheduling blocks (SB) where the
   number of measurement sets (MS) is different to the number of
   beams. This addresses an issue where the SB have recorded 36 MSs
   but only a subset of them are valid. 
 * The use of dcp for copying MSs from the archive is turned off by
   default to minimise the load on the hpc-data nodes (the method for
   doing this is not ideal). 

Processing Software:
 * Reduction in logging in the imager task. 
 * Modifications to Selavy to include additional information in the
   headers of the spectra & related images (Object name, date-obs and
   duration, Project ID and SBID, history comments). This involved
   improvements to the image interface classes. 
 * Fixed a problem where mslist output was corrupted by long field
   names. 
 * Shortened objectID strings are now used in catalogues. No longer
   uses image name, but instead SBID + catalogue/data product type +
   sequence ID.   


0.19.7 (11 January 2018)
------------------------

A patch release that allows the pipelines to run correctly on native
slurm, using srun to launch applications rather than aprun. This is
timed to be available for the upgrade of the galaxy supercomputer to
CLE6.

The release also has a slightly improved build procedure that better
handles python dependencies, and updated documentation regarding the
ASKAP processing platform at Pawsey.

No functional change is expected for the processing software itself.


0.19.6 (19 November 2017)
-------------------------

A patch release for both the processing and pipeline areas. This fixes
a few bugs and introduces a few minor features to enhance the
processing.

Pipelines:
 * Default values of a number of parameters have been updated,
   particularly for the spectral-line imaging. Importantly, the
   default imager has been changed *for all imaging jobs* to be the
   new imager task.
 * Fix for the image-based continuum subtraction script. This uses
   scripts in the ACES repository, which have been recently updated,
   and this change allows the use of the new interface. Needs to be
   used with ACES revision number 47195 or later.
 * The bandpass solutions can now be applied to the calibrator
   observations themselves, producing calibrated MSs that could be
   used later for analysis.
 * The reference antenna for the bandpass calibration can be specified
   via the new config parameter BANDPASS_REFANTENNA.
 * Self-calibration with cmodel can now avoid using components below
   some nominated signal-to-noise level. It can also be forced to use
   PSF-shaped components for the calibration.
 * When copying raw per-beam measurement sets, there is now the option
   to use regular cp, instead of the dcp-over-ssh approach (which
   requires the ability to ssh to hpc-data).
 * The first stage of mosaicking now uses the weighttype=Combined
   option (see below), which should give a better reflection of the
   data in the event different beams have different weights. Previous
   behaviour can be used by setting the config parameter
   LINMOS_SINGLE_FIELD_WEIGHTTYPE=FromPrimaryBeamModel.
 * The following bugs have been fixed:

   * RM Synthesis is now turned off if only the Stokes-I continuum
     cube is being created (which is the default).
   * When using a component parset for self-calibration, the reference
     direction could be incorrect (if the full-resolution MS was
     absent). This has been fixed, by obtaining the direction from the
     averaged dataset.
   * The continuum source-finding will now not attempt to measure
     spectral terms of higher order than the number of terms requested
     in the imaging (for instance, if nterms=2, the spectral curvature
     will not be measured). Similarly, in that situation the .taylor.2
     images will not be provided as mosaics or as final archived
     artefacts.

Processing software:

 * Cflag:

   * There was a bug where the StokesV flagger would crash with a
     segmentation fault on occasions where it was presented with a
     spectrum or time-series that was entirely flagged. It is now more
     robust against such datasets.

 * Imager:

   * The imager is now more robust against small changes in the
     frequency labels of channels, with an optional tolerance
     parameter available.
     
 * Selavy:
   
   * A few bugs were fixed that were preventing Selavy working for
     spectral-line cubes, where it was trying to read in the entire
     cube on all processing cores (leading to an out-of-memory error).
   * Moment-0 maps now have a valid mask applied to them.
   * Selavy can now measure the spectral index & curvature from a
     continuum cube, instead of fitting to Taylor-term images.
   * Duchamp version 1.6.2 has been included in the askapsoft
     codebase.
   * The deconvolved position angle of components is now forced to lie
     between 0 & 2pi, and its error is limited to be no more than 2pi.
     
 * Linmos:
   
   * Fixed a bug that meant (in some cases) only a single input image
     was included in the mosaic. Happened when the input images had
     masks attached to them (for instance, combination of mosaics).
   * New option of "weighttype=Combined" for linmos-mpi, that uses
     both the weight images and the primary beam model to create the
     output weights.
   


0.19.5 (8 October 2017)
-----------------------

A patch release that adds a few new bits of functionality:

The Selavy code has been updated to add to the catalogue
specifications for the continuum island & component catalogues:

 * The component catalogue now has error columns for the deconvolved
   sizes, as well as for the alpha & beta values.
 * Additionally, the 3rd flag column now indicates where the alpha &
   beta values are measured from - true indicates they come from
   Taylor-term images.
 * The island catalogue now has:
   
   * An error column for the integrated flux density
   * Columns describing the background level, both the mean background
     across the island, and the average background noise level.
   * Statistics for the residual after subtracting the island's fitted
     Gaussian components - columns for the max, mean, min, standard
     deviation and rms.
   * Columns indicating the solid angle of the island, and of the
     image restoring beam.
     
 * Occasional errors in converting the major/minor axis sizes to the
   correct units have also been fixed.

The pipelines have been updated with new functionality and options:
 * The new ingest mode of recording one measurement set per beam is
   now able to be processed. The MS metadata is recorded from one of
   the measurement sets, and the splitting is done from the
   appropriate beam. For the science dataset, if no selection of
   channels or scans is required, and there is only a single field in
   the observation, then copying of the MS is done instead of
   splitting.
 * Stokes-V flagging is available for all flagging steps. This is
   performed in the same job as the dynamic amplitude flagging, and is
   parameterised by its own parameters - consult the documentation for
   the full list (essentially the same as FLAG_DYNAMIC parameters with
   STOKESV replacing DYNAMIC or DYNAMIC_AMPLITUDE).
 * Selection of specific spectral channels in the flagging tasks is
   now possible with CHANNEL_FLAG_1934, CHANNEL_FLAG_SCIENCE, and
   CHANNEL_FLAG_SCIENCE_AV. 
 * A bug that meant the continuum source-finding job would fail to
   convert higher-order Taylor terms or continuum cubes to FITS format 
   has been fixed.
 * A fix has been made to the bandpass-smoothing casa script call,
   adding in a --agg command-line flag to the casa arguments. This
   allows the plotting to be run correctly on the compute nodes.
 * Scripting errors in the flagging scripts that showed up when
   splitting was not being done have been rectified.


0.19.4 (21 September 2017)
--------------------------

A patch release covering the pipeline scripts and the processing
software. The following bugs are fixed:

 * The pipeline configuration parameter FOOTPRINT_PA_REFERENCE will
   now over-ride the value of footprint.rotation in the scheduling
   block parset. Additionally, the scheduling block summary metadata
   files (created in the pipeline working directory) are now not
   regenerated if they already exist.
 * The metadata collection in the pipeline now does not fail if a
   FIELD in the measurement set has 'RA' in its name.
 * There was a memory leak in Selavy, causing an error to be thrown
   when dealing with fitted components, specifically when the
   numGaussFromGuess flag was set to false and a fit failed. The code
   now falls back to whatever the initial estimate for components was,
   even if that has fewer than the maximum number indicated by
   maxNumGauss.
 * There was a half-pixel offset enforced in the location of the
   fitted Gaussian when fitting to the restoring beam when
   imaging. This was resulting in a slightly incorrect restoring
   beam.
 * If there are multiple MSs in the SB directory, one can be processed
   by giving MS_INPUT_SCIENCE its full path, setting the SB_SCIENCE
   parameter appropriately, and putting DIR_SB="".

0.19.3 (4 September 2017)
-------------------------

A patch release just covering the pipeline scripts. The following bugs
are fixed:

 * The number of writers used in the spectral-line imaging when the
   askap_imager is used (DO_ALT_IMAGER=true) is now better
   described. The input parameter NUM_SPECTRAL_CUBES is now
   NUM_SPECTRAL_WRITERS, and the pipeline is better able to handle a
   single output (FITS) cube written by multiple writers.
 * The running of the validation script after continuum source-finding
   now has the $ACES environment variable set correctly. The
   validation script requires it to be set, and when it was
   not set within a user's environment the script could crash.
 * The image-based continuum subtraction script has had two fixes:
   
   * The cube name was being incorrectly set when the single-writer
     FITS option was used
   * The working directory was the same for all sub-bands for a given
     beam. This could cause issues with casa's ipython log file,
     resulting in jobs crashing with obscure errors.

0.19.2 (24 August 2017)
-----------------------

A patch release that fixes bugs in both the pipeline scripts and
Selavy, as well as a minor one in casdaupload.

Pipeline fixes:
 * The 'contsub' spectral cubes were not being mosaicked. This was
   caused by incorrect handling of the ".fits" suffix (it was being
   added for CASA images, not FITS image).
 * It was possible for the pipeline to attempt to flag an averaged MS
   even if the averaged MS was not being created. The pipeline is now
   more careful about setting its switches to cover this scenario.
 * The continuum validation reports are now automatically (by default)
   copied to a standard location, tagged with the user's ID and
   timestamp of pipeline. This can be turned off by setting
   VALIDATION_ARCHIVE_DIR to "".
 * The spectral imaging jobs were capable of asking for more writers
   than there were cores in the job. The pipeline scripts are now
   careful to check the number of writers, and ensure it is no more
   than the number of workers. The default number of writers has been
   changed to one.
 * The handling of FITS files by the inter-field mosaicking tasks was
   error-prone - files would either not be copied (in the case of a
   single field) or would not be identified correctly (for the
   spectral-line case).

Pipeline improvements:
 * The image size (number of pixels) and cellsize (in arcsec) for the
   continuum cubes can now be given explicitly, and so be allowed to
   differ from the continuum images.
 * Some default cleaning parameters for continuum cube imaging have
   been changed as well.


The following bugs in Selavy have been fixed:
 * There was an issue with the weight-normalisation option in Selavy,
   where the incorrect normalisation was applied if a subsection (in
   particular the first subsection) had no valid pixels present
   (ie. all were masked). The masking is now correctly accounted for.
 * There were bugs that caused memory errors in the spectral-line (HI)
   parameterisation of sources. This code has been improved.
 * The 'fitResults' files were reporting the catalogue twice, and
   producing the same catalogue for all fit types. Additionally, there
   was the possibility of errors if different fit types yielded
   different numbers of components for a given island. 

Finally, the casdaupload utility would fail if presented with a
wildcard that did not resolve to anything. It will now just carry on,
ignoring that particular parameter.


0.19.1 (04 August 2017)
-----------------------

User documentation changes only. No code changes.


0.19.0 (06 July 2017)
---------------------

New features:

 * linmos now produces mosaicks with correct masking of pixels in in
   both CASA and FITS formats.
 * linmos can also remove the contribution of the primary beam
   frequency dependence to the Taylor term images. This only applies
   to Gaussian primary beam models.
 * Added Selavy support for FITS outputs
 * Addition of ACES-OPS module to facilitate controlled dependency
   between ASKAPsoft and ACES Tools.
 * Parallelised the RM Synthesis module in Selavy.
 * New Selavy output - a map of the residual emission not covered by
   the fitted Gaussians in a continuum image.
 * Developed patch for casacore's poor handling of the lanczos
   interpolation method.
 * Added support for casdaupload to handle spectral-line catalogues.
 * CASDA related Support for new image types.
 * Ensure calibration tables are uploaded to CASDA.
 * Added support for continuum validation script and results including
   CASDA upload.
 * Improvements to Selavy spectral-line parameterisation.
 * Selavy sets spectral index & curvature to a flag-value if not
   calculated rather than leaving as zero.
 
Bug fixes:

 * linmos, reduced memory footprint. A bug was found that was causing
   a complete image cube to accessed, when only the image shape was
   required. This has been fixed. 
 * Selavy catalogues occasionally fail CASDA validation due to wide
   columns - fixed.
 * Fixed bug where restore.beam.cutoff value not read from parset when
   present.
 * Added missing beam log output to new imager.
 * Improved handling of failed processing and the effect of that on
   executing final diagnostics/FITSconversion/thumbnails jobs at end
   of pipeline.
 * Use number of beams in footprint rather than assume 36.
 * Minor bug fixes

0.18.3 (23 May 2017)
--------------------

This patch release fixes the following bugs in the pipeline scripts:

 * Incorrect indexing of some self-calibration array parameters
 * Better handling of logic in determining the usage of the
   alternative imager.
 * Ensuring the image-based continuum-subtracted cubes are converted
   to FITS and handled by the CASDA upload. Also that this task is
   able to see cubes directly written to FITS by the spectral
   imagers. 
 * Fixing handling of directory names so that extracted artefacts are
   found correctly for FITS conversion.
 * Removal of extraneous inverted commas in the continuum imaging
   jobscript.

Additionally, there is a new parameter USE_CLI, which defaults to true
but allows the user to turn off use of the online services, should
they not be available.

Finally, a number of the default parameters used by the bandpass
calibration and the continuum imaging have been updated, following
extensive commissioning work with the 12-antenna early science
datasets. Here is a list of the changed parameters:

.. code-block:: bash
                
   NCYCLES_BANDPASS_CAL=50
   NUM_CPUS_CBPCAL=216
   BANDPASS_MINUV=200
   BANDPASS_SMOOTH_FIT=0
   BANDPASS_SMOOTH_THRESHOLD=3.0
   NUM_TAYLOR_TERMS=1
   NUM_PIXELS_CONT=3200
   CELLSIZE_CONT=4
   RESTORING_BEAM_CUTOFF_CONT=0.5
   GRIDDER_OVERSAMPLE=5
   CLEAN_MINORCYCLE_NITER=4000
   CLEAN_PSFWIDTH=1600
   CLEAN_SCALES="[0]"
   CLEAN_THRESHOLD_MINORCYCLE="[40%, 1.8mJy]"
   CLEAN_NUM_MAJORCYCLES="[1,8,10]"
   CLEAN_THRESHOLD_MAJORCYCLE="[10mJy,4mJy,2mJy]"
   PRECONDITIONER_LIST="[Wiener]"
   PRECONDITIONER_GAUSS_TAPER="[10arcsec, 10arcsec, 0deg]"
   PRECONDITIONER_WIENER_ROBUSTNESS=-0.5
   RESTORE_PRECONDITIONER_LIST="[Wiener]"
   RESTORE_PRECONDITIONER_GAUSS_TAPER="[10arcsec, 10arcsec, 0deg]"
   RESTORE_PRECONDITIONER_WIENER_ROBUSTNESS=-2
   SELFCAL_NUM_LOOPS=2
   SELFCAL_INTERVAL="[57600,57600,1]"
   SELFCAL_SELAVY_THRESHOLD=8
   RESTORING_BEAM_CUTOFF_CONTCUBE=0.5
   RESTORING_BEAM_CUTOFF_SPECTRAL=0.5

0.18.2 (5 May 2017)
-------------------

This patch release fixes the following bugs in the pipeline scripts:

 * The ntasks-per-node parameter for the continuum subtraction could
   still be more than ntasks for certain parameter settings.
 * When using a subset of the spectral channels, the new imager jobs
   were not configured properly, with some elements trying to use the
   full number of channels.
 * Mosaicking of the image-based-continuum-subtracted cubes was not
   waiting for the completion of the continuum subtraction jobs, so
   would invariably fail to run correctly. 
 * The image-based continuum-subtraction jobs are now run from
   separate directories, so that ipython logs can not conflict.
 * The spectral source-finding job had an error in the image name in
   the parset.
 * Mosaicking of the continuum-cubes now creates separate weights
   cubes for each type of image product.
 * Continuum imaging with the new imager has been improved, fixing
   inconsistencies in the names of images.
 * The PNG thumbnails were not being propagated to the CASDA
   directory. 

The noise map produced by Selavy is now included in the set of
artefacts converted to FITS and sent to CASDA. 

Additionally, the ability to impose a position shift to the model used
in self-calibration has been added, with the aim of supporting
on-going commissioning work.

0.18.1 (13 April 2017)
----------------------

This patch release sees a few bug-fixes to the pipeline scripts:

 * When re-running the pipeline on already-processed data, where the raw input
   data no longer exists in the archive directory, the pipeline was previously
   failing due to it not knowing the name of the MS or the related metadata
   file. It now has the ability to read MS_INPUT_SCIENCE and MS_INPUT_1934 and
   determine the metadata file from that. It will also not try to run jobs that
   depend on the raw data.
 * The new imager used in spectral-line mode can now be directed to create a
   single spectral cube, even with multiple writers, via the
   ALT_IMAGER_SINGLE_FILE and ALT_IMAGER_SINGLE_FILE_CONTCUBE parameters.
 * There have been changes to the defaults for the number of cores for spectral 
   imaging (from 2000 to 200) and the number of cores per node for continuum
   imaging (from 16 to 20), based on benchmarking tests.
 * In addition, the following bugs were fixed:

   * The ntasks-per-node parameter could sometimes be more than ntasks, causing
     a slurm failure.
   * The self-calibration algorithm was not retaining images from the
     intermediate loops.
   * The image-based continuum subtraction script was not finding the correct
     image cube.


0.18.0 (29 March 2017)
----------------------

New features and updates:

 * Scheduling block state changes, in conjunction with a new TOS
   release:
   
   * The CP manager now monitors the transition from EXECUTING to
     OBSERVED, and the ICE interfaces have been updated accordingly.
   * The pipeline will now transition the scheduling block state from
     OBSERVED to PROCESSING at the beginning of processing. This will
     only be done for scheduling blocks in the OBSERVED state, and
     will apply to both the science field and the bandpass calibrator.
     
 * Python libraries:
   
   * 3rdParty python libraries have been updated to current
     versions. This applies to: numpy, scipy, matplotlib, pywcs, pytz,
     and APLpy. The current astropy package has been added, and pyfits
     has been removed. The python scripts in Analysis/evaluation have
     been updated to be consistent with these new packages.
   * There is a new script in Analysis/evaluation,
     makeThumbnailImage.py, that produces grey-scale plots of
     continuum images, and has the capability to add weights contours
     and/or continuum components. This script is used by the
     makeThumbnails script in the pipeline, as well as the new
     diagnostics script (that produces more complex plots aimed at
     being aids for quality analysis).
     
 * Calibration & Imaging changes:
   
   * The residual image is now the residual at the end of the last
     major cycle. (Previously, it was the residual at the beginning of
     the last major cycle.)
   * The residual images now have units of Jy/beam rather than
     Jy/pixel, and have the restoring beam written to the header.
   * When the "restore preconditioner" option is used in imaging, the
     residual and psf.image are also written out for this
     preconditioner.
     
 * Pipeline updates:
   
   * There is a new pipeline parameter, CCALIBRATOR_MINUV, that allows
     the bandpass calibration to exclude baseline below some value.
   * Minor errors and inconsistencies in some catalogue specifications
     have been fixed, with the polarisation catalogue being updated to
     v0.7.
   * The spectral-line catalogue has been added to the CASDA upload part
     of the pipeline, and has been renamed to incorporate the image name
     (in the line of other data products).
   * There are new pipeline parameters SELFCAL_REF_ANTENNA &
     SELFCAL_REF_GAINS that allow the self-calibration to use a
     reference antenna and/or gain solution.
   * A weights cutoff for Selavy can now be specified via the config
     file using the new parameters SELAVY_WEIGHTS_CUTOFF &
     SELAVY_SPEC_WEIGHTS_CUTOFF (rather than using the linmos cutoff
     value).
   * The new imager is better integrated into the pipeline, with
     DO_ALT_IMAGER parameters for CONT, CONTCUBE & SPECTRAL.
   * It is possible to make use of the direct FITS output in the
     pipeline, by using "IMAGETYPE_xxx" parameters for CONT, CONTCUBE &
     SPECTRAL. Note that this is still somewhat of a
     work-in-progress.

Bug fixes:

 * Casacore v2 had several patches added that had been left out of the
   upgrade. Notably a patch allowing the use of the SIGMA_SPECTRUM
   measurement set column following concatenation of measurement
   sets.
 * The mssplit utility has been made more robust with memory allocation
   when splitting large datasets.
 * Better checking of the size of SELFCAL- and imaging-related arrays
   in the pipeline configuration, particularly when not using
   self-calibration.
 * [Weights bug in Selavy]
 * The continuum-subtracted cubes were not being mosaicked by the
   pipeline.
 * The pipeline is more robust against errors encountered when
   obtaining the metadata at the beginning. It can better detect when
   a corrupted metadata file is present, and re-run the extraction of
   that metadata.
 * An error in handling the beam numbering for non-zero beam numbers
   was identified & fixed.
 * The pipeline Selavy jobs were using the incorrect weights cutoff,
   leading to them not searching the full extent of the image.
 * The use of the PURGE_FULL_MS flag in the pipelines will now not
   trigger the re-splitting (and subsequent processing) of the
   full-resolution dataset.


0.17.0 (24 February 2017)
-------------------------

New features:

 * Capability for direct FITS output from imager. The "fits" imagetype
   is now supported for cimager and imager. This should be considered "beta"
   as the completeness of the header information for post processing has not
   been confirmed. This enables the parallel write of FITS cubes which considerably
   improves the performance of spectral line imaging.
 * Selavy's RM Synthesis module can export the Faraday Dispersion
   Function to an image on disk.
 * New source-finding capabilities in the processing pipelines, with a
   spectral-line source-finding task added (using Selavy), and the
   option of RM Synthesis done in the continuum source-finding.
 * The full-resolution measurement set can be purged by the pipeline
   when no longer needed (ie. after the averaging has been done, and
   if no spectral-line imaging is required). This will help to
   minimise unncessary disk-space usage.
 * CASDA upload is now able to handle extracted spectral data products
   (object spectra and moment maps etc) that are produced by the
   source-finding tasks.
 * A few relatively minor additions have been made to the pipeline
   scripts:
   
   * A minimum UV distance can be applied to the bandpass calibration.
   * The checks done on the self-calibration parameters are less
     restrictive and less prone to give warning messages.
   * Mosaicking at the top level (combining FIELDs) is now not done
     when there is only a single FIELD.
     
 * User documentation has been updated to better reflect the current
   arrangements with Pawsey (e.g. host names and web addresses). It
   also describes new modules that are available, as well as
   alternative visualisation options using Pawsey's zeus cluster.

Bug fixes:

 * Imaging:
   
   * The brightness units in the restored images from the new imager are
     now correctly assigned (they were 'Jy/pixel' and are now
     'Jy/beam'). The beam is also now written correctly.
   * The beam logs (recording the restoring beam at each channel of an
     image cube) are now read correctly - previously the comment line
     at the start was not being ignored.
   * A number of fixes for the spectral line imaging mode of "imager"
     have been implemented. These fix issues with zero channels caused
     by flagging.

* Analysis:
  
   * The Faraday Dispersion function in Selavy's RM Synthesis module
     was being incorrectly normalised. It is now normalised by the
     model Stokes I flux at the reference frequency.
     
 * Pipelines:
   
   * When using more than one Taylor term in the imaging, the continuum
     subtraction with cmodel images was not working correctly, with
     incomplete subtraction. This was due to a malformed parset
     generated within the pipeline. This has been fixed, and the
     continuum subtraction works as expected.
   * The beam logs are now correctly passed to Selavy for accurate
     flux correction of extracted spectra.
   * Job dependencies for the mosaicking and source-finding jobs have
     been fixed, so that all jobs start when they are intended to. The
     mosaicking jobs now only start when they are needed, to avoid
     wasting resources.
   * The project ID was incorrectly obtained from the schedblock
     service when there was more than one word in the SB alias.
   * The SELAVY_POL_WRITE_FDF parameter was incorrectly described in
     the documentation - it has been renamed
     SELAVY_POL_WRITE_COMPLEX_FDF.


0.16.1 (16 December 2016)
-------------------------

A patch release that is largely bug fixes, with several minor
updates to the pipeline scripts.

New features:

 * The pipelines will now accept a list of beams to be processed, via
   a comma-separated list of beams and beam ranges - for instance
   0,1,4,7-9,16. This should be given with the BEAMLIST configuration
   parameter. If this is not given, it falls back to using BEAM_MIN &
   BEAM_MAX as usual.
 * An additional column is now written to the stats files, showing the
   starting time of each job.
 * There is a new parameter FOOTPRINT_PA_REFERENCE that allows a user
   to specify a reference rotation angle for the beam footprint,
   should it not be included in the scheduling block parset.
 * There is a new parameter NCHAN_PER_CORE_SPECTRAL_LINMOS that
   determines how many cores are used for the spectral-line
   mosaicking. This helps ensure that the job is sized such that the
   memory load is spread evenly.

Bug fixes:

 * Imaging:
   
   * Improvements to the new imager to handle writers who do not get
     work due to the barycentring.
   * Improvements to the allocation of work within the new imager.
     
 * RM Synthesis & Selavy:
   
   * The new RM Synthesis module was not correctly respecting the '%p'
     wildcard in image names, which also affected extraction run from
     within Selavy. This has been fixed.
     
 * Pipelines:
   
   * The findBandpass slurm job had a bug that stopped it completing
     successfully.
   * A number of bugs were identified with the mosaicking:
     
     * The Taylor term parameter was set incorrectly in the continuum
       mosaicking scripts.
     * The image name was not being set correctly in the spectral-line
       mosaicking.
     * The job dependencies for the spectral-line mosaicking have been
       fixed so that all spectral imaging jobs are included.
       
   * The askapsoft module is now loaded more reliably within the slurm
     jobs.
   * The return value of the askapcli tasks is now tested, so that
     errors (often due to conflicting modules) can be detected and the
     pipeline aborted.
   * A certain combination of parameters (IMAGE_AT_BEAM_CENTRES=false
     and DO_MOSAIC=false) meant that the determination of fields in
     the observation was not done, so no science processing was
     done. This has been fixed so that the list of fields is always
     determined.
   * A couple of bugs in the source-finding script were fixed, where
     the image name was incorrectly parsed, and the Taylor 1 & 2
     images were not being found.
   * The footprint position angle for individual fields was
     incorrectly being added to the default value listed in the
     scheduling block parset.
   * To avoid conflicts between source-finding results of different
     images, the artefacts produced by selavy (catalogues and images)
     now incorporate the image name in their name. The source-finding
     jobs are also more explicit in which image they are searching.
   * Finally, two deprecated scripts have been removed from the
     pipeline directory.


0.16.0 (28 November 2016)
-------------------------

A release with a number of bug fixes, new features, and updates to the
pipeline scripts

New features:

 * Rotation Measure synthesis is now possible within the Selavy
   source-finder. This extracts Stokes spectra from continuum cubes at
   the positions of identified continuum components, performs RM
   Synthesis, and creates a catalogue of polarisation properties for
   each component. While still requiring some development, most
   features are available and should permit testing.
 * The new imager, which was made available in an earlier release, has
   been added to the askapsoft module at Pawsey.

Bug fixes for processing software:

 * The bandpass calibrator cbpcalibrator will now not allow through a
   bandpass table with NaN values in it. If NaNs appear in solving the
   bandpass, then cbpcalibrator will throw an exception. In the
   process, the GSL library used in 3rdParty has been updated to v1.16.
 * The writing of noise maps by Selavy (in the VariableThreshold case)
   has been streamlined, so that making such maps for large cubes is
   more tractable.

Pipeline updates:

 * The driving script for the ASKAP pipeline is now called
   processASKAP.sh, instead of processBETA.sh. The latter is still
   available, but gives a warning before callling processASKAP.sh. All
   interfaces remain the same.
 * Linear mosaicking has been improved:
   
   * It is now available for spectral-line and continuum cubes, in
     addition to continuum images.
   * Mosaics are made for each field, and for each tile if the
     observation was done with the "tilesky" mode.
   * The continuum mosaicking can also include mosaics of the
     self-calibration loops.
     
 * The pipelines make better use of the online services of ASKAP, to
   determine things like the footprint (location of beams). This makes
   calculations more internally self-consistent.
 * When running self-calibration, some parameters can be given
   different values for each loop. This includes parameters for the
   cleaning, the source-finding, and the calibration. More flexibility
   is also provided for the source-finding within the self-calibration.
 * Processing of BETA datasets are made possible via an IS_BETA
   parameter, which avoids using the online system to obtain beam
   locations, and changes the defaults for the data location.
 * Smoothing of the bandpass solutions is now possible, using a script
   in the ACES repository to produce a new calibration table. It also
   allows plotting of the calibration solutions.
 * More flexibility is allowed for the number of cores used in the
   continuum imaging.
 * A notable bug was fixed that led to incorrect calibration and
   continuum-subtraction when Taylor-terms were being produced
   (i.e. nterms>1)
 * Various other more minor bug fixes, related to logging, stats
   files, and default values of parameters (for instance, the default
   for cmodel was to use a flux cutoff that was too high).


0.15.2 (26 October 2016)
------------------------

This is a patch release that fixes several issues:

 * The parallel linear mosaicking tool linmos-mpi has been patched to
   correct a bug that was initialising cube slices incorrectly.
 * Several fixes to the CP manager and the pipeline scripts were made
   following end-to-end testing with the full ASKAP online system:
   
   * The CP manager will send notifications to a nominated JIRA ticket
     upon SB state changes.
   * Several fixes were made to the CASDA uploading and polling
     scripts, to ensure accurate execution. The capability of sending
     notifications to a JIRA ticket has also been added.
   * The Project ID is now taken preferentially from the SB, rather
     than the config file.
   * The linear mosaicking in the pipelines is now not turned off when
     only a single beam is processed.


0.15.1 (19 October 2016)
------------------------

This is a patch release that fixes a couple of issues:

 * The bandpass calibrator cbpcalibrator has had its run-time improved
   by changing the way the calibration table is written. It is now
   written in one pass at the completion of the task - this reduces
   the I/O overhead and greatly reduces the run-time for larger
   datasets.
 * The pipeline settings for the flagging have been changed. The
   default settings now are to have the integrate_spectra option
   switched on, and the integrate_times and flat amplitude options
   switched off. This is the same approach as used in 0.14.0-p2 and
   earlier, and so should avoid the case of most of the dataset being
   flagged (as was seen with ADE data using the default settings in
   0.15.0).
 * The flagging step for the average dataset now uses a different
   check-file to the full-size dataset flagging.


0.15.0 (10 October 2016)
------------------------
This release sees a number of bugs fixes and improvements.

* Improved the efficiency of the msmerge operation by allowing the
  writing of arbitrary tile-sizes and the mssplit by forcing bulk
  read operations from the source measurement set when possible.
* To be consistent with changes made to Cimager (ASKAPSDP-1607),
  Simager has been changed to only access cross-correlations.
* Parallel linmos - a new application linmos-mpi with the same
  interface as linmos has been added. This will distribute the channels
  of the cube between mpi ranks and process them separately. Writing each
  channel to the output cube individually. This should allow a full
  resolution cube to be mosaicked.
* Improved Selavy HI emission catalogue, with a more complete set of
  parameters available. This is now turned on by an input parameter
  Selavy.HiEmissionCatalogue.
* JIRA notification for Scheduling Block status changes.
* Pipeline updates:
  
  * The bandpass calibration approach has changed slightly. All beams
    of the calibrator will be processed up to the requested BEAM_MAX -
    the BEAM_MIN parameter only applies to the science dataset.
  * There is more flexibility in specifying flagging thresholds for
    the dynamic flagger. Each instance of the flagging can have
    different thresholds for the integrateSpectra & integrateTimes
    options, and both of these are now available for the bandpass
    calibrator.
  * When uploading to CASDA and upon successful ingest into CASDA, the
    SB state can be transitioned through the state model.
  * Initial support for the new imager.
    
* Modified CBPCALIBRATOR to reference the XX and the YY visibilities
  independently to the XX and YY of the reference antenna.
* Added ability to playback in any number of loops in Correlator
  and TOS Simulators.

Bug fixes:
 * Pipelines:
   
   * When components were used in the pipeline for self-calibration or
     continuum subtraction, the reference direction was not being
     interpreted correctly, leading to erroneous positions.
   * The bandpass calibration table was not inheriting the complete
     path to it - it is now put in a standard location and all scripts
     correctly point to it.
   * More robustness added to the source-finding job so that it
     doesn't run if the FITS conversion fails.
     
 * Documentation fixes to names of the MS utility functions.
 * Fixing casdaupload to handle images that don't have associated
   thumbnails, and to set the correct write permissions of the upload
   directory.
 * Selavy's extraction of moment maps and cubelets was not working
   correctly when a subsection was given to Selavy. These calculations
   have also been improved slightly to better handle the spectral
   increments.
 * Minor-fixes to new imager to deal with brittle logic in the channel
   allocations in spectral line mode. My fix for this essentially gives
   all the workers the same info as the master.


0.14.0-p2 (25 September 2016)
-----------------------------

A further update only to the pipeline processing:

 * Changes to the directory structure created by the pipeline. Each
   field in the MS is given its own directory, within which processing
   on all beams is done. The bandpass calibrator likewise gets its own
   directory. All files & job names are now identified by the field
   and the beam IDs.
 * Flagging of the science data is now done differently. The MS is
   first bandpass-calibrated, and then flagged. After averaging, there
   is the option to run the flagging again on the averged data. The
   flagging for the bandpass calibrator has not been changed.
 * The dynamic flagging for the science data also allows the use of
   both integrateSpectra and integrateTimes, with the former no longer
   done by default.
 * Modules are loaded correctly by the scripts and slurm jobs before
   particular tasks are used, so that the scripts are less reliant on
   the user's environment.
 * Better handling of metadata files, particularly if a previous
   metadata call had failed.
 * The FITS conversion and thumbnail tasks correctly interact with the
   different fields, and the thumbnail images make a better
   measurement of the image noise, taking into account any masked
   regions from the associated weights images.
 * The cleaning parameter Clean.psfwidth is exposed to the
   configuration file.
 * Bugs in associating the footprint information with the correct
   field have been fixed.
 * If the CASDA-upload script is used to prepare data for deposit, the
   scheduling block state is transitioned to PENDINGARCHIVE.



0.14.0-p1 (9 September 2016)
----------------------------

An update to the pipeline processing only:

 * Fixing a bug in the handling of multiple FIELDs within a
   measurement set. These are now correctly given their own directory
   for the processed data products.
 * The footprint parameters are now preferentially determined from the
   scheduling block parset (using the 'schedblock' command-line
   utility). If not present, the scripts fall back to using the config
   file inputs.
 * The metadata files (taken from mslist, schedblock and footprint.py)
   are re-used on subsequent runs of the pipeline, rather than
   re-running each of these tools.
 * The default bucketsize for the mssplit jobs has been increased to
   1MB, and made configurable by the user. The stripe count for the
   non-data directories has also been changed to 1.


0.14.0 (11 August 2016)
-----------------------

A major release, with several new features and improvements for both
the imaging software and the pipeline scripts.

A new imager in under test in this release, currently just called
"imager" and it has the following features:

 * In continuum mode it allows a core to process more than one channel.
   This has a small cost in memory and a proportional increase in disk
   access. But allows the continuum imaging to proceed with a much smaller
   footprint on the cluster. This will allow simultaneous processing of all
   beams in a coming release.
 * Spectral line cubes can be made from measurement sets that are from different
   epochs. The epochs are imaged separately but merged into the same image for
   minor-cycle solving.
 * The output spectral line cubes can be in the barycentric frame. This is currently
   just nearest neighbour indexing. But the possibility of interpolation has not been
   designed out.
 * The concept of "multiple writers" has been introduced to improve the disk access
   pattern for the spectral line mode.  This breaks up the cube into frequency bands.
   These can be recombined post-processing.
 * If you really want to increase the performance for many major cycles you can
   also turn on a shared memory option which stores visibility sets in memory throughout
   processing.
 * The imager takes the same parset as Cimager - but extra key-value pairs are required to implement
   the features.

This new imager is still under test and we have not added the hooks into the pipeline yet.

Other updates to the imaging code include:
 * Simager is now more robust against completely-flagged
   channels - such channels will now be set to zero in the output
   cube, instead of failing the simager job.
 * The extraction of spectra done by Selavy is now more robust and
   better able to handle multiple components and distributed
   processing.
 * Selavy now accepts a reference direction when providing a
   components parset - the l & m coordinates are calculated relative
   to this, rather than the image centre.
 * The restore solver can now accept its own preconditioner
   parameters, in addition to the general parameters used by the
   other solvers. If specified, a second set of restored images
   will be written with suffix ".alt.restored".

The pipeline scripts have seen the following updates:
 * There is a new option to have a different image centre for each
   beam, rather than a common pixel grid for all images. This uses the
   beam centre location taken from the footprint.py utility (an
   external task in the ACES subversion area).
 * The self-calibration can now use cmodel to generate a model image,
   instead of using a components parset.
 * There are new tasks to:
   
   * Apply the gains calibration to the averaged measurement set
   * Image the averaged measurement set as "continuum cubes", in
     multiple polarisations
   * Apply an image-based continuum-subtraction following the creation
     of the spectral-line cubes. This makes use of an ACES python
     script to fit a low-order polynomial to each spectrum in the
     cube.
     
  * The headers of the FITS files created by the pipelines now have a
    wider range of metadata, including observatory and date-obs
    keywords, as well as information about the askapsoft & pipeline
    versions.
  * The restore preconditioner options mentioned above are available
    through "RESTORE_PRECONDITIONER_xxx" parameters, for the continuum
    imaging only (it is not implemented for simager).
  * Several bugs were fixed:
    
    * The continuum subtraction was failing when using components if
      no sources were found - it now skips the continuum subtraction
      step.
    * The askapdata module was, in certain situations, not loaded
      correctly, leading to somewhat cryptic errors in the imaging.
    * The parsing of mslist to obtain MS metadata would sometimes
      fail, depending on the content of the MS. It is now much more
      robust.
    * The default for TILENCHAN_SL has been increased to 10, to
      counter issues with mssplit running slow.


0.13.2 (19 July 2016)
---------------------

This bug-fix version addresses a few issues with the imaging &
source-finding code, along with minor updates to the pipeline
scripts.
The following bugs have been fixed in the processing software:

 * Caching of the Wiener preconditioner is now done, so that the
   weights are only calculated once for all solvers and the filters
   are only calculated once for all major cycles, scales &
   Taylor-terms. This has the effect of greatly speeding up the
   imaging, particularly for large image sizes.
 * The BasisfunctionMFS solver has had the additional convolution with
   the PSF removed. This fixes a bug where central sources were being
   cleaned preferentially to sources near the edge of the image.
   It also improves the resolution and SNR of minor-cycle dirty images.
 * From the update to casacore-2 in 0.13.0, linmos would fail when
   mosaicking images without restoring beams. This has been fixed (and
   behaves as it did prior to 0.13.0).
 * The size check in Selavy that rejects very large fitted components
   has been re-instated. This should allow the rejection of spurious
   large fitted components. The minimum size requirement (which forced
   sizes to be >60% of the PSF) has been removed.

And the pipeline has seen these fixes:
 * The resolution of the input science measurement set, when not given
   explicitly in the config file, is now done properly in all cases,
   rather than just for the case of splitting & flagging.
 * The pipeline now allows clipping in the snapshot option of the
   gridding - this improves performance at high declinations, where
   different warping between snapshots could introduce sharp edges to
   the weights image.
 * The pipeline also allows the use of a weights cutoff in the Selavy
   job used in self-calibration, to avoid the presence of these sharp
   cutoffs seen at high declinations.


0.13.1 (24 June 2016)
---------------------

This bug-fix version primarily addresses issues with the processing
pipelines. The following bugs have been fixed:

 * Non-integer image cell sizes were not being interpreted
   correctly. These values can now be any decimal value.
 * A change in the mslist output format with casacore v2 meant that
   the Cmodel continuum subtraction script was not reading the correct
   reference frequency. This caused the cmodel job to fail for the
   case of nterms>1. The parsing code has been fixed.
 * The archiving scripts had a few changes:
   
   * The resolution of filenames & paths has been fixed.
   * The source-finding is now run on FITS versions of the images
   * The catalogue keys in the observation.xml are now internally
     consistent.
   * The way thumbnail sizes are specified in the pipeline
     configuration file has changed slightly.

Related to the above changes, the C++ code has had a couple of
changes:

 * casdaupload now correctly puts the thumbnail information in the
   <image> group in the observation.xml file.
 * Fixes were made to the Selavy VOTable output to fix formatting
   errors that were preventing it passing validation upon CASDA
   ingest.

Other C++ code changes include:
 * Fixes to the output files from the crossmatch utility.
 * Updates to the slice interfaces for compatibility with the TOS.

The documentation has also been updated, with updated descriptions of
parameters that have changed as a result of the above, a few typos
fixed, and new information about the management of data on Pawsey's
scratch2 filesystem.

0.13.0 (31 May 2016)
--------------------

This version fixes a few issues with the processing pipelines, fixes
some bugs with the source-finder and casda upload utility, and moves
the underlying code to use version 2 of the casacore package.

The pipeline scripts have seen the following changes:
 * The requested times for the slurm jobs are now individually
   configurable via parameters in the processBETA config file.
 * The Pawsey account can be explicitly given, allowing the use of the
   scripts under other accounts on magnus.
 * The linmos job now properly checks the CLOBBER parameter, and will
   avoid over-writing mosaicked images if CLOBBER=false.
 * There is now an archiving option to the pipeline, which includes:
   
   * conversion of images to FITS format
   * creation of PNG 'thumbnail' versions of the 2D images
   * staging of data to a directory for ingest into CASDA

The processing software had the following changes:
 * The casacore package has been updated to version 2.0.3, with
   corresponding changes throughout the ASKAPsoft code tree. 
 * NOTE that this has resulted in the code not building on OS X
   Mavericks (10.9). 
 * The Selavy sourcefinder had two changes:
   
   * Errors on the fitted parameters are now reported in the component
     catalogue.
   * A bug that stopped Selavy running the variable-threshold option
     when the SNR image name was not specified has been fixed.
     
 * The casdaupload utility now requires the observation start and end
   times to be specified if no measurement set is provided.


0.12.2 (24 May 2016)
--------------------

A bug fix release for the processing pipeline.
This fixes a problem where the mosaicking task was still assuming beam
IDs that had a single integer - ie. it was looking for
image.beam0.restored instead of image.beam00.restored.


0.12.1 (18 May 2016)
--------------------

This is a simple patch release that fixes a couple of bugs, one of
which affected the performance of both the source-finder and the
pipelines.

The measurement of spectral indices for fitted components to continuum
Taylor-term images was being done incorrectly, leading to erroneous
values for spectral-index and spectral-curvature. This, in turn, could
lead to inaccuracies or even failures in the continuum-subtraction
task of the pipeline (when the CONTSUB_METHOD=Cmodel option was used).
This only affected version 0.12.0 (released on 8 May 2016), and is
fully corrected in 0.12.1.

The other bug enforces the total number of channels processed by the
pipelines to be an exact multiple of the averaging width
(NUM_CHAN_TO_AVERAGE). In previous versions, the pipeline scripts
would press on, but this would potentially result in errors in the
slurm files and jobs not executing. Now, should NUM_CHAN_TO_AVERAGE
not divide evenly into the number of channels requested, the script
will exit with an error message before submitting any jobs.

0.12.0 (8 May 2016)
-------------------

This version has a number of changes to the processing applications
and the pipeline scripts.

Bugs that have been fixed in the processing applications include:
 * The deconvolution major cycles were using out-of-date residual
   values when logging and testing against the threshold.majorcycle
   parameter. This is now fixed.
 * The initialisation of calibrator input now depends more closely on
   the input parameters nAnt, nBeam & the calibrator model, rather
   than the first chunk of the data - this allows the shape of the
   data cube to change throughout the dataset (which will help with
   data imported from MIRIAD/CASA).
 * Simager was showing a cross-shaped artefact when Wiener
   preconditioning was used, even with the preservecf parameter set to
   true. This parameter is now recognised, and the artefact is no
   longer seen.
 * Full polarisation handling is now possible with simager (in the
   same manner as for cimager).
 * Simager was crashing when no preconditioner was given - this has been fixed.
 * The casdaupload task now conforms to the current CASDA requirements
   of allowing multiple SBIDs, and of reporting the image type.
 * Selavy's Gaussian fitting is now more able to fit confused
   components that are not immediately identified from the initial
   estimates. 
 * Selavy was also failing when given images of a particular name
   (short, without a full-stop). This has been fixed. 

The pipeline scripts have had a number of improvements:
 * They are more robust for processing ADE data, with >9 beams and >6 antennas.
 * The flagging tasks have been improved, with:
   
   * Flagging of autocorrelations an option
   * The selection flagger (that does antenna-based &
     autocorrelations) is done first, along with (an optional) flat
     amplitude threshold. 
   * The dynamic flagging is done as the second pass
   * There is more user control over these individual elements
     
 * New parameters are available in the scripts, to make use of the
   snapshotimaging.longtrack parameter in the gridding, and
   normalisegains option in the self-calibration. The latter improves
   the performance of the self-calibration, approximating phase-only
   self-calibration.
 * The slurm jobfiles are now more robust to the user's environment -
   if the askapsoft module has not been loaded, it will be in the
   jobfile, and the user can request a different version. 


0.11.2 (28 March 2016)
----------------------

This release is a relatively small bug-fix update, primarily fixing a
bug in cimager.

This bug would prevent a parallel job completing in the case of the
major cycle threshold being reached prior to the requested maximum
number of major cycles.

Other changes include:
 * The pipeline scripts have a few minor fixes to the code to improve
   reliability, and ensure the correct number of cores used for jobs
   is reported in the statistics files.
 * The only change to the ingest pipeline (within askapservices)
   incorporates an extra half-cycle wait following fringe-rotator
   update. 


0.11.1 (8 March 2016)
---------------------

The imaging software now incorporates the preservecf option (released
in 0.11.0) into the SphFunc gridder, and introduces a new option to
the gridding - snapshotimaging.longtrack - that predicts the best fit
W plane used for the snapshot imaging, finding the plane that
minimises the future deviation in W. This can have substantial savings
in processing time for long tracks.

The pipeline scripts have seen a number of minor improvements and
fixes, with improved alternative methods for continuum subtraction,
and improved reporting of resource usage (including a record of the
number of cores used for each job). The user configuration file is now
also copied to a timestamped version for future reference.

The ingest pipeline code has incorporated changes resulting from the
recent commissioning activities.


0.11.0 (15 February 2016)
-------------------------

A key change made in the processing software relates the
preconditioning. There is a new parameter preconditioning.preservecf
that should be set to true for the case of using WProject and the
Wiener preconditioner. This has fixed a couple of issues - at low
(negative) robustness values, the cross-shaped artefact that was
sometimes seen has now gone, and the performance should now more
closely match that expected from robust weighting for the full range
of robustness values.

Several other bugs were fixed:
 * Linmos had a bug (that was introduced in version 0.10) where
   automatically-generated primary beams were being set to the
   position of the first image. 
 * The multiscale-MFS solver had a small bug that would lead to
   higher-order terms being preconditioned multiple times. 
 * Cmodel had bugs related to the reading of Selavy catalogues, and
   correctly representing deconvolved Gaussians. It now works
   correctly with such data.
 * Simager would fail were no preconditioners supplied.
 * Selavy now better handles images that do not have spectral axes (an
   issue when dealing with images made by packages other than ASKAPsoft).

Additionally, the regridding has been sped up through a patch to the
casacore library.

The pipeline scripts also have a new feature, making use of Selavy +
Cmodel to better perform the continuum subtraction from spectral-line
data. The old approach is still available, but is not the default.


0.10.1 (18 January 2016)
------------------------

Much of this release relates to updates to the ingest pipeline and
related tasks, in preparation for getting it running at Pawsey. These
are now deployed as their own module, although it is not expected that
ACES members will need to use this.

In the science processing area, an important fix was made to the code
responsible for uvw rotations. A fault was identified where these were
being projected into the wrong frame, which could lead to positional
offsets in images made away from the initial phase centre. This fault
has been fixed.

Some initial fixes to the preconditioner have been made that may
improve images when Wiener filtering with a low or negative robustness
parameter. Improvements are only expected when snapshot imaging is not
being used. A full fix is being tested and is planned for the next
release.

This release also sees the BETA pipeline scripts move into an
askapsoft-derived module (although this had previously been
announced).


0.9.0 (12 October 2015)
-----------------------

There are only a small number of changes to the core processing part
of the software that would affect ACES work on galaxy, and these are
almost all to do with the source-finder Selavy. The default values of
some parameters governing output files have changed, with the
preference now to minimise the number of output files. A few
corrections have been made to the units of parameters in some of the
output catalogues.


0.8.1 (10 September 2015)
-------------------------

This release introduces simager, the prototype spectral-line imager -
this allows imaging of large spectral cubes through distributed
processing, and is capable of creating much larger cubes than
cimager. While this is not the final version of the spectral-line
imager - the software framework that underpins the imaging code is
going through a re-design prior to early science - it does demonstrate
the distributed-processing approach that enables large numbers of
spectral channels to be processed.

For those wanting to make use of the ACES scripts under subversion,
these will be updated shortly to include use of simager.

Other changes to the askapsoft module include minor updates to the
CASDA HI catalogue interface from the Selavy sourcefinder, and
ADE-related updates to the ingest pipeline and associated tools (which
won’t affect work on galaxy).


0.7.3 (21 August 2015)
----------------------

This release has a few relatively small bug fixes that have been
resolved in the past week:

 * a minor fix to cimager that solves a rare problem with the
   visibility metadata statistics calculations, that would result in
   cimager failing (this had been seen in processing the basic
   continuum tutorial data).
 * correcting the shape (BMAJ/BMIN/BPA) parameters in the
   Selavy-generated component parset output (that might be used as
   input to ccalibrator in self-calibration) - they were previously
   given in arcsec/degrees rather than radians (as required by
   ccalibrator/csimulator). 
 * aligning the cmodel VOTable inputs with the new Selavy output formats
 * a fix to the units in one of the Selavy VOTable outputs 


0.7.2 (9 August 2015)
---------------------

This release is a bug-fix release aimed at fixing a problem identified
in running the basic continuum imaging tutorial. There was an issue
with the way the simulated data had been created, which meant that
mssplit would fail on those measurement sets. This has been fixed
(fixing both mssplit and msmerge), and the tutorial dataset and
description have been updated.

If you use mssplit on real BETA data, you will not notice any
difference, save for potentially a small performance improvement.

The only other change has been implementation of the CASDA format for
absorption-line catalogues, although the implementation of actual
absorption-line searching is not complete in Selavy, so this will
probably not affect any of you (it has been more to provide early
examples for use by the CASDA team).


0.7.0 (3 July 2015)
-------------------

The key features of the release are:
 * Mk-II compatible ingest (although not applicable for galaxy processing)
 * A new task mslist that provides basic information for a measurement
   set
 * Phase-only calibration

Bug:
 * [ASKAPSDP-1657] - mssplit corrupts POINTING table
 * [ASKAPSDP-1658] - change actual_pol to expect degrees as the unit
 * [ASKAPSDP-1660] - Driving to an AzEl position throws an exception in the ingest pipeline.

Feature:
 * [ASKAPSDP-1635] - SupportSearcher performance patch
 * [ASKAPSDP-1650] - Develop utility to extract and print information from a measurement set
 * [ASKAPSDP-1670] - Develop phase-only calibration option for CImager

Task:
 * [ASKAPSDP-1663] - Modify ingest pipeline source task to conform with the ADE correlator ioc changes



0.6.3 (11 May 2015)
-------------------

Changes for this release include bug fixes and improvements to assist
the casdaupload tool, and a calibration bug that affected leakage
terms. The release notes follow.

Bug
 * [ASKAPSDP-1665] - Data format bug in casdaupload

Feature
 * [ASKAPSDP-1659] - Update casdaupload utility to conform to new spec

Task
 * [ASKAPSDP-1633] - Test ASKAPsoft leakage calibration using BETA observation 619
 * [ASKAPSDP-1668] - Fix width and precision in CASDA catalogues


0.6.1 (12 March 2015)
---------------------

A bug-fix release adding a couple of elements to 0.6.0:

Bug
 * [ASKAPSDP-1657] - mssplit corrupts POINTING table
 * [ASKAPSDP-1658] - change actual_pol to expect degrees as the unit



0.6.0 (6 March 2015)
--------------------

Some highlight features and bugfixes are:

 * [ASKAPSDP-1652] - Gridding failing with concatenated MS
 * [ASKAPSDP-1654] - Selavy's component parset output gets positions wrong
 * [ASKAPSDP-1646] - Develop CASDA upload utility
 * [ASKAPSDP-1649] - Add selection by field name to mssplit
 * [ASKAPSDP-1653] - Add parset parameter to change the weight cutoff used in linmos


Bug
 * [ASKAPSDP-1628] - ASKAPsoft fails to build on Ubuntu 14.04
 * [ASKAPSDP-1632] - Spurious message: Observation has been aborted before first scan was started
 * [ASKAPSDP-1642] - Intermittant functest failure in java-logappenders
 * [ASKAPSDP-1651] - Program version string shows "Unknown" branch name
 * [ASKAPSDP-1652] - Gridding failing with concatenated MS
 * [ASKAPSDP-1654] - Selavy's component parset output gets positions wrong

Feature
 * [ASKAPSDP-1615] - Implement Ice monitoring interface in Ingest Pipeline
 * [ASKAPSDP-1637] - Flag antennas with out-of-range delays
 * [ASKAPSDP-1638] - Adapt VOTable output of Selavy to match recent CASDA table descriptions
 * [ASKAPSDP-1646] - Develop CASDA upload utility
 * [ASKAPSDP-1649] - Add selection by field name to mssplit
 * [ASKAPSDP-1653] - Add parset parameter to change the weight cutoff used in linmos

Task
 * [ASKAPSDP-1624] - Document ASKAPsoft SDP platform dependencies
 * [ASKAPSDP-1640] - Update user documentation to use /scratch2 filesystem
 * [ASKAPSDP-1641] - Update Scons dependency to 2.3.4



0.5.1 (9 January 2015)
----------------------

A bug fix release, providing an option to flag antennas with
out-of-range delays in the DRx or FR hardware setting.


0.5.0 (15 December 2014)
------------------------

The list of features & bugfixes is below:

Bug
 * [ASKAPSDP-1606] - Segmentation fault when using cflag dynamic threshold
 * [ASKAPSDP-1608] - Calibration fails when flagged visibilities have values of NaN or Inf
 * [ASKAPSDP-1616] - Row index calculation in Ingest Pipelines MergedSource::addVis() is too slow
 * [ASKAPSDP-1622] - CP Manager should gracefully handle unavailability of the FCM

Feature
 * [ASKAPSDP-1607] - Change the default for data accessor parameter "CorrelationType"
 * [ASKAPSDP-1610] - Account for averaging when setting noise sigma values in mssplit
 * [ASKAPSDP-1612] - Add support for SIGMA_SPECTRUM column to Data Accessor
 * [ASKAPSDP-1623] - Ingest Pipeline: Add support for pausing an observation with scanid -1

Task
 * [ASKAPSDP-1603] - Improve scalability of (spectral-line) source-finding
 * [ASKAPSDP-1611] - Remove 3rdParty/mysql dependency
 * [ASKAPSDP-1613] - Document cpmanager
 * [ASKAPSDP-1630] - Update Apache Ant dependency to 1.9.4


0.4.1 (13 November 2014)
------------------------

A minor update, with the following features added:

 * [ASKAPSDP-1610] - Account for averaging when setting noise sigma values in mssplit
 * [ASKAPSDP-1612] - Add support for SIGMA_SPECTRUM column to Data Accessor


0.4.0 (22 October 2014)
-----------------------

The list of features & bugfixes is below:

Bug
 * [ASKAPSDP-1567] - ccalapply running slow
 * [ASKAPSDP-1570] - AdviseParallel fails when run in parallel with the tangent parameter unset
 * [ASKAPSDP-1578] - Ingest pipeline fails with exception in FrtHWAndDrx
 * [ASKAPSDP-1581] - CP manager occasionally fails to mkdir
 * [ASKAPSDP-1587] - Selavy - remove limits on component ID suffix
 * [ASKAPSDP-1589] - cimager fails when direction not specified
 * [ASKAPSDP-1594] - Thresholds in Selavy get too low near the edge caused by low weights
 * [ASKAPSDP-1596] - cbpcalibrator crashes in parallel mode
 * [ASKAPSDP-1598] - Typo in VOTable PARAM headers

Feature
 * [ASKAPSDP-1390] - Develop ASKAP imaging advise functionality
 * [ASKAPSDP-1551] - Add time based selection to MSSplit
 * [ASKAPSDP-1569] - AdviseParallel should distribute statistics back to the workers
 * [ASKAPSDP-1573] - Add dynamic threshold flagging to cflag
 * [ASKAPSDP-1580] - Support AZEL coordinate system in ingest pipeline
 * [ASKAPSDP-1582] - Add timing metrics in ingest pipeline
 * [ASKAPSDP-1588] - Add ability for Selavy to write out a component parset
 * [ASKAPSDP-1592] - Obtain linmos feed centres from a reference image
 * [ASKAPSDP-1599] - Implement Ice monitoring interface in CP Manager
 * [ASKAPSDP-1600] - Add scan id to vispublisher

Task
 * [ASKAPSDP-1583] - Improve performance of Ingest FlagTask
 * [ASKAPSDP-1584] - FringeRotationTask needs some performance improvements


0.3.0 (28 July 2014)
--------------------

The version 0.3 release of the ASKAPsoft Science Data Processor has
been installed as a module to Galaxy. The included features/bugfixes
are listed below, and are also listed on Redmine:
https://pm.atnf.csiro.au/askap/projects/cmpt/versions/197

 * Bug #6029: Ingest pipeline zeros flagged visibilities
 * Bug #6107: Fix the curvature-map option in Selavy's Gaussian fitting
 * Bug #6112: Ingest pipeline flags incorrect antenna
 * Bug #6113: RA & Dec swapped in Ingest Pipeline Monitoring data
 * Bug #6121: openssl-1.0.1c fails to build on XUbuntu 14.04
 * Bug #6125: Superfluous loop over w in WProjectVisGridder::initConvolutionFunction
 * Bug #6126: gridder parameter snapshotimaging.coorddecimation is ignored
 * Bug #6154: Ingest pipeline should not write SBID in observation column
 * Bug #6179: SVN 1.7 breaks rbuilds get_svn_revision function
 * Bug #6183: Selavy - component catalogues for individual fit types are incomplete
 * Feature #6073: Support of different phase and pointing centres via scheduling blocks
 * Feature #6075: MSSink should populate POINTING table
 * Feature #6120: Ingest Pipeline: Get obs data from TOS metadata
 * Feature #6164: Tool to assist delay calibration
 * Feature #6180: Add --version cmdline parameter to askap::Application
 * Task #6176: SDP codebase restructure
 * Documentation #6106: Create an analysis tutorial


0.2.0 (4 June 2014)
-------------------

Bug
 * [ASKAPSDP-1522] - Inappropriate default level of logging in CP applications
 * [ASKAPSDP-1523] - cpingest: NaNs in visibilities
 * [ASKAPSDP-1526] - Selavy: source lists differ between serial & distributed processing
 * [ASKAPSDP-1529] - Problems when running Selavy on FITS file
 * [ASKAPSDP-1533] - ccalibrator ignores the data for other than the first beam in the antennagain mode

Feature
 * [ASKAPSDP-1261] - Integrate CP ingest pipeline with TOS
 * [ASKAPSDP-1540] - Handle scan id of -2 in ingest pipeline

Task
 * [ASKAPSDP-1525] - Update Duchamp to 1.6
 * [ASKAPSDP-1537] - ASKAPsoft SDP - Cleanup HPC build environment


0.1.0 (31 March 2014)
---------------------

Feature
 * [ASKAPSDP-1459] - Develop linmos utility
 * [ASKAPSDP-1460] - ccalibrator enhancements

Task
 * [ASKAPSDP-1521] - Create CP-0.1 release
