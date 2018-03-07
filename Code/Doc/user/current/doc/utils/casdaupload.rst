Casda Upload Utility
====================

The CASDA upload utility prepares artifacts for submission to the CSIRO ASKAP
Science Data Archive (CASDA). Currently the following artifact types are
supported:

* Images (FITS format only)
* Source Catalogues (VOTable format only)
* Visibilities (CASA Measurement Set format only)
* Evaluation Pipeline Artifacts (any file format)

The utility takes as input one or more of the above file types, plus a
configuration parameter set. It performs the following tasks:

* Creates a directory, relative to a given base directory, where all artifacts
  will be deposited for ingest into the CASDA system
* Produces a metadata file (XML) that enumerates the artifacts to be archived.
  This file is read by the CASDA ingest software
* Creates a tarfile of the measurement set. The measurement set is a directory,
  where CASDA requires a single file per artifact
* Creates a checksum file for each artifact. This file contains three strings,
  each separated by a single space character:
 
  - First is a CRC-32 checksum of the content displayed as a 32 bit lower case
    hexadecimal number
  - Second is the SHA-1 of the content displayed as a 160 bit lower case
    hexadecimal number
  - Third is  the size of the file displayed as a 64 bit lower case hexadecimal
    number

* Deposits all artifacts, the checksum files and the metadata file in the
  designated output directory, unless absolute paths are given in the
  XML file.
* Finally, and specifically as the last step, a file named "READY" can
  be written to the output directory. This indicates no further
  addition or mutation of the data products in the output directory
  will take place and the CASDA ingest process can begin. This is only
  done if the *writeREADYfile* parameter is set to *true*.

Running the program
-------------------

The utility can be run with the following command, where "config.in" is a file
containing the configuration parameters described in the next section. ::

    casdaupload -c config.in

Configuration Parameters
------------------------

The configuration file contains a section that declares general information
pertaining to the observation, followed by the  declaration of specific
artifacts to be archived. See the 

The required parameters are:

+-----------------------------+----------------+-----------------+------------------------------------------------+
|**Parameter**                |**Type**        |**Default**      |**Description**                                 |
+=============================+================+=================+================================================+
|outputdir                    |string          |None             |Base directory where artifacts will be          |
|                             |                |                 |deposited. A directory will be created within   |
|                             |                |                 |the "outputdir" named per the "sbid" parameter  |
|                             |                |                 |described below. For example, if the "outputdir"|
|                             |                |                 |is /foo/bar and the "sbid" is 1234 then the     |
|                             |                |                 |directory /foo/bar/1234 will be created and all |
|                             |                |                 |artifacts, plus the metadata file, will be      |
|                             |                |                 |copied there.                                   |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|useAbsolutePaths             |bool            |true             |Whether paths to the artifacts are given as     |
|                             |                |                 |absolute paths (true), or as simple filenames in|
|                             |                |                 |the output directory. If true, the artifacts    |
|                             |                |                 |with absolute paths will *not* be copied to the |
|                             |                |                 |output directory. However, even if true, the    |
|                             |                |                 |measurement set tarballs will still be copied.  |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|telescope                    |string          |None             |Name of the telescope that produced the         |
|                             |                |                 |artifacts                                       |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|sbid                         |string          |None             |Primary scheduling block id for the observation |
|                             |                |                 |that these artifacts relate to                  |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|sbids                        |vector of       |None             |List of all scheduling block ids that contribute|
|                             |strings         |                 |to the observation. The primary sbid, given by  |
|                             |                |                 |sbid (above) is *not* listed in this list, and  |
|                             |                |                 |the primary must be the lowest-number SBID.     |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|obsprogram                   |string          |None             |Observation program which the scheduling block  |
|                             |                |                 |relates to                                      |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|obsStart                     |string          |None             |Start time of the observation. This needs to be |
|                             |                |                 |in 'FITS' format YYYY-MM-DDThh:mm:ss (note the  |
|                             |                |                 |'T' in the middle), where the time is in        |
|                             |                |                 |UTC. This parameter needs to be given if there  |
|                             |                |                 |are no measurement sets provided, but if there  |
|                             |                |                 |are, it is ignored.                             |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|obsEnd                       |string          |None             |End time of the observation, with formatting as |
|                             |                |                 |for obsStart.                                   |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|writeREADYfile               |bool            |false            |A flag indicating whether to write the READY    |
|                             |                |                 |file to the output directory. For now, the      |
|                             |                |                 |default is not to write it, meaning it is up to |
|                             |                |                 |the user to set the READY file so that CASDA    |
|                             |                |                 |knows to import the data.                       |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|images.artifactlist          |vector<string>  |None             |(Optional) A list of keys defining image        |
|                             |                |                 |artifact entries that appear in the parameter   |
|                             |                |                 |set                                             |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|catalogues.artifactlist      |vector<string>  |None             |(Optional) A list of keys defining catalogue    |
|                             |                |                 |artifact entries that appear in the parameter   |
|                             |                |                 |set                                             |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|measurementsets.artifactlist |vector<string>  |None             |(Optional) A list of keys defining measurement  |
|                             |                |                 |set artifact entries that appear in the         |
|                             |                |                 |parameter set                                   |
+-----------------------------+----------------+-----------------+------------------------------------------------+
|evaluation.artifactlist      |vector<string>  |None             |(Optional) A list of keys defining evaluation   |
|                             |                |                 |artifact entries that appear in the parameter   |
|                             |                |                 |set                                             |
+-----------------------------+----------------+-----------------+------------------------------------------------+

Then for each artifact declared within any of the "artifactlists" the
following parameter set entries must be present:

+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|**Parameter**                |**Type**        |**Default**      |**Description**                                                                 |
+=============================+================+=================+================================================================================+
|<key>.filename               |string          |None             |Filename (either relative or fully qualified with a path) for the artifact      |
|                             |                |                 |                                                                                |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|<key>.type                   |string          |None             |This is only valid for catalogue or image artifacts. This refers to the type of |
|                             |                |                 |catalogue or image being uploaded. For catalogues, it must be one of the        |
|                             |                |                 |following: 'continuum-island', 'continuum-component', 'polarisation-component', |
|                             |                |                 |'spectral-line-emission' or 'spectral-line-absorption'. The full list of image  |
|                             |                |                 |types can be found at                                                           |
|                             |                |                 |https://confluence.csiro.au/display/CASDA/Stage+1.5+Analaysis+of+Image+Types    |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|<key>.format                 |string          |pdf              |This is only valid for evaluation artifacts. This defines the format that the   |
|                             |                |                 |file in question is presented in. Accepted values of format are: 'pdf', 'txt',  |
|                             |                |                 |'validation-metrics', 'calibration' or 'tar'.                                   |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|<key>.project                |string          |None             |The project identifier (OPAL code) to which this artifact is allocated for      |
|                             |                |                 |validation. For the evaluation artifacts this parameter may be present, however |
|                             |                |                 |it is ignored since evaluation reports are not subject to validation.           |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|<imagekey>.thumbnail_large   |string          |None             |(Optional) This parameter, only used for images, indicates the filename of the  |
|                             |                |                 |large thumbnail image. This parameter is not mandatory.                         |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+
|<imagekey>.thumbnail_small   |string          |None             |(Optional) This parameter, only used for images, indicates the filename of the  |
|                             |                |                 |small thumbnail image. This parameter is not mandatory.                         |
+-----------------------------+----------------+-----------------+--------------------------------------------------------------------------------+

An image element may have a set of extracted spectral data products
associated with it. These could be integrated spectra of detected
objects, noise spectra surrounding an object, moment maps of a 3D
(spectral-line) source, or Rotation Measure Synthesis outputs.

These have the property of allowing the filename and thumbnail
filenames to be specified with a wildcard, that is resolved at
run-time.

The following parameters are used to denote spectra and moment
maps. They have the same syntax for each individual set of
artifacts. Note the hierarchical structure of the parameters, and see
below for examples.

+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|**Parameter**                  |**Type**        |**Default**      |**Description**                                                               |
+===============================+================+=================+==============================================================================+
|<imagekey>.spectra             |vector<string>  |None             | (Optional) A list of keys defining sets of spectral files to be              |
|                               |                |                 | uploaded. These are referred to as <key> in the rows below.                  |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|<imagekey>.momentmaps          |vector<string>  |None             | (Optional) A list of keys defining sets of moment-map images to be           |
|                               |                |                 | uploaded. These are referred to as <key> in the rows below.                  |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|<imagekey>.cubelets            |vector<string>  |None             | (Optional) A list of keys defining sets of cubelets (cut-outs from the larger|
|                               |                |                 | spectral cube) to be uploaded. These are referred to as <key> in the rows    |
|                               |                |                 | below.                                                                       |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|<imagekey>.<key>.filename      |string          |None             | Filename (either relative or fully qualified with a path) for the            |
|                               |                |                 | artifact. This may contain wildcards.                                        |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|<imagekey>.<key>.type          |string          |None             | This refers to the type of spectral artifact being uploaded.                 |
|                               |                |                 | The full list of spectral types can be found at                              |
|                               |                |                 | https://confluence.csiro.au/display/CASDA/Stage+2+Analysis+of+Spectral+types |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+
|<imagekey>.<key>.thumbnail     |string          |None             | (Optional) This parameter indicates the filename of the thumbnail            |
|                               |                |                 | image. This parameter can use wildcards. It is not mandatory, but, if given, |
|                               |                |                 | must resolve to the same number of files as the filename.                    |
+-------------------------------+----------------+-----------------+------------------------------------------------------------------------------+

Examples
~~~~~~~~

As an example of declaring artifacts, the below defines two image artifacts, a
deconvolved (Cleaned) image and a PSF image:

.. code-block:: bash

    # Declares two images
    images.artifactlist = [img, psfimg]

    img.filename        = image.clean.i.restored.fits
    img.project         = AS007
    psfimg.filename     = psf.image.i.clean.fits
    psfimg.project      = AS007


The following example declares four artifacts to be uploaded to CASDA, one for
each of the artifact types: image, source catalogue, measurement set and evaluation
report.

.. code-block:: bash

    # General
    outputdir   = /scratch2/casda
    telescope   = ASKAP
    sbid        = 1234
    sbids       = [1235,1236]
    obsprogram  = test

    # Images
    images.artifactlist             = [image1]

    image1.filename                 = image.i.dirty.restored.fits
    image1.type                     = cont_restored_2d
    image1.project                  = AS007
    image1.thumbnail_small          = image.i.dirty.restored_small.png

    # Source catalogues
    catalogues.artifactlist         = [catalogue1]

    catalogue1.filename             = selavy-results.components.xml
    catalogue1.type                 = continuum-component
    catalogue1.project              = AS007

    # Measurement sets
    measurementsets.artifactlist    = [ms1]

    ms1.filename                    = 2014-12-20_021255.ms
    ms1.project                     = AS007

    # Evaluation reports
    evaluation.artifactlist         = [report1]

    report1.filename                = evaluation-report.pdf
    report1.format                  = pdf


Finally, here is an example where an image has a number of extracted
object and noise spectra associated with it. The wildcards provided
would match the following images: pol_spec_I_1a.fits, pol_spec_I_2a.fits,
pol_spec_I_3a.fits; with associated thumbnails pol_spec_I_1a.png etc.

.. code-block:: bash
    
    # General
    outputdir                       = /scratch2/casda
    telescope                       = ASKAP
    sbid                            = 1234
    sbids                           = [1235,1236]
    obsprogram                      = test
    writeREADYfile                  = true
    
    # Images - continuum cube and spectral cube
    images.artifactlist             = [image1,image2]
    image1.filename                 = image.i.contcube.sb1234.linmos.restored.fits
    image1.type                     = cont_restored_3d
    image1.project                  = AS033
    image1.spectra                  = [spec,noise]
    image1.spec.filename            = selavy_image.i.contcube.sb1234.linmos.restored/PolData/pol_spec_I*.fits
    image1.spec.type                = cont_restored_i
    image1.spec.thumbnail           = selavy_image.i.contcube.sb1234.linmos.restored/PolData/pol_spec_I*.png
    image1.noise.filename           = selavy_image.i.contcube.sb1234.linmos.restored/PolData/pol_noise_I*.fits
    image1.noise.type               = cont_noise_i
    #
    image2.filename                 = image.i.cube.sb1234.linmos.restored.fits
    image2.type                     = spectral_restored_3d
    image2.spectra                  = [spec,noise]
    image2.spec.filename            = selavy_image.i.cube.sb1234.linmos.restored/Spectra/spectrum*.fits
    image2.spec.type                = cont_restored_i
    image2.spec.thumbnail           = selavy_image.i.cube.sb1234.linmos.restored/Spectra/spectrum*.png
    image2.noise.filename           = selavy_image.i.cube.sb1234.linmos.restored/Spectra/noiseSpectrum*.fits
    image2.noise.type               = cont_noise_i   
    image2.momentmaps               = [mom0,mom1,mom2]
    image2.mom0.filename            = selavy_image.i.cube.sb1234.linmos.restored/Moments/moment0*.fits 
    image2.mom0.type                = spectral_restored_mom0
    image2.mom1.filename            = selavy_image.i.cube.sb1234.linmos.restored/Moments/moment1*.fits 
    image2.mom1.type                = spectral_restored_mom1
    image2.mom2.filename            = selavy_image.i.cube.sb1234.linmos.restored/Moments/moment2*.fits 
    image2.mom2.type                = spectral_restored_mom2
    image2.cubelets                 = [cubelet]
    image2.cubelet.filename         = selavy_image.i.cube.sb1234.linmos.restored/Cubelets/cubelet*.fits 
    image2.cubelet.type             = spectral_restored_3d
