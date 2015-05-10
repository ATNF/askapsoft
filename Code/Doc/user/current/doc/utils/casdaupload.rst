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
  designated output directory
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

+-----------------------------+----------------+-----------------+----------------------------------------------+
|**Parameter**                |**Type**        |**Default**      |**Description**                               |
+=============================+================+=================+==============================================+
|outputdir                    |string          |None             |Base directory where artifacts will be        |
|                             |                |                 |deposited. A directory will be created within |
|                             |                |                 |the "outputdir" named per the "sbid" parameter|
|                             |                |                 |described below. For example, if the          |
|                             |                |                 |"outputdir" is /foo/bar and the "sbid" is 1234|
|                             |                |                 |then the directory /foo/bar/1234 will be      |
|                             |                |                 |created and all artifacts, plus the metadata  |
|                             |                |                 |file, will be copied there.                   |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|telescope                    |string          |None             |Name of the telescope that produced the       |
|                             |                |                 |artifacts                                     |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|sbid                         |string          |None             |Scheduling block id for the observation that  |
|                             |                |                 |these artifacts relate to                     |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|obsprogram                   |string          |None             |Observation program which the scheduling block|
|                             |                |                 |relates to                                    |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|writeREADYfile               |bool            |false            |A flag indicating whether to write the READY  |
|                             |                |                 |file to the output directory. For now, the    |
|                             |                |                 |default is not to write it, meaning it is up  |
|                             |                |                 |to the user to set the READY file so that     |
|                             |                |                 |CASDA knows to import the data.               |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|images.artifactlist          |vector<string>  |None             |(Optional) A list of keys defining image      |
|                             |                |                 |artifact entries that appear in the parameter |
|                             |                |                 |set                                           |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|catalogues.artifactlist      |vector<string>  |None             |(Optional) A list of keys defining catalogue  |
|                             |                |                 |artifact entries that appear in the parameter |
|                             |                |                 |set                                           |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|measurementsets.artifactlist |vector<string>  |None             |(Optional) A list of keys defining measurement|
|                             |                |                 |set artifact entries that appear in the       |
|                             |                |                 |parameter set                                 |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|evalation.artifactlist       |vector<string>  |None             |(Optional) A list of keys defining evalation  |
|                             |                |                 |artifact entries that appear in the parameter |
|                             |                |                 |set                                           |
+-----------------------------+----------------+-----------------+----------------------------------------------+

Then for each artifact declared within any of the "artifactlists" the
following parameter set entries must be present:

+-----------------------------+----------------+-----------------+----------------------------------------------+
|**Parameter**                |**Type**        |**Default**      |**Description**                               |
+=============================+================+=================+==============================================+
|<key>.filename               |string          |None             |Filename (either relative or fully qualified  |
|                             |                |                 |with a path) for the artifact                 |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|<key>.type                   |string          |None             |This is only valid for catalogue              |
|                             |                |                 |artifacts. This refers to the type of         |
|                             |                |                 |catalogue being uploaded. It must be one of   |
|                             |                |                 |'continuum-island', 'continuum-component' or  |
|                             |                |                 |'polarisation-component'.                     |
+-----------------------------+----------------+-----------------+----------------------------------------------+
|<key>.project                |string          |None             |The project identifier to which this artifact |
|                             |                |                 |is allocated for validation. For the          |
|                             |                |                 |evaluation artifacts this parameter may be    |
|                             |                |                 |present, however it is ignored since          |
|                             |                |                 |evaluation reports are not subject to         |
|                             |                |                 |validation.                                   |
+-----------------------------+----------------+-----------------+----------------------------------------------+

As an example of declaring artifacts, the below defines two image artifacts, a
deconvolved (Cleaned) image and a PSF image:

.. code-block:: bash

    # Declares two images
    images.artifactlist = [img, psfimg]

    img.filename        = image.clean.i.restored.fits
    img.project         = AS007
    psfimg.filename     = psf.image.i.clean.fits
    psfimg.project      = AS007


Example
~~~~~~~

The following example declares four artifacts to be uploaded to CASDA, one for
each of the artifact types: image, source catalogue, measurement set and evaluation
report.

.. code-block:: bash

    # General
    outputdir   = /scratch2/casda
    telescope   = ASKAP
    sbid        = 1234
    obsprogram  = test

    # Images
    images.artifactlist             = [image1]

    image1.filename                 = image.i.dirty.restored.fits
    image1.project                  = AS007

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
