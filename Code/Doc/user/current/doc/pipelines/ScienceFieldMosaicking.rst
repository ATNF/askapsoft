User Parameters - Mosaicking
============================

Levels of Mosaicking
--------------------

The default mode of the pipeline is to produce mosaics at several
levels for a given scheduling block. Each field in the measurement set
will have its own mosaic, combining all requested beams for that
field.

When the scheduling block has used the *tilesky* mode, the fields in the
measurement set will be of the form *NAME_Tx-yI*, where *x* & *y* give
the offsets in the tiling grid, and *I* is one of A,B,C,D, indicating
the interleaving offset for that tile position. *NAME* is some
identifying text. In this situation, each *x-y* tile will have its own
mosaic, where the mosaics of each interleave position for that tile
are combined.

Finally, the mosaics of all fields and/or tiles are combined to form a
single mosaic image for the scheduling block. This second stage of
mosaicking is not done if there is only a single field in the
measurement set.

Additionally, if there are multiple fields in the measurement set, but
they should *not* be mosaicked together (for instance, if they are
quite separate locations on the sky), then you can set
``DO_MOSAIC_FIELDS=false``. Then the individual fields will be
mosaicked but nothing more.

The mosaicking is done for all image product types -- continuum
images, continuum cubes, and spectral-line cubes. For continuum
images, the **linmos** tool is used, while the **linmos-mpi** tool is
used for the cubes, allowing parallelism over the spectral channels.

For the continuum imaging case when self-calibration has been used, a
mosaic can be made of each loop of the self-calibration process. This
is done for each field (ie. each interleave position of each tile),
and can be turned on/off with the parameter ``MOSAIC_SELFCAL_LOOPS``.


Beam locations
--------------

The linmos tools require beam offsets to know where to place the
primary beam models. For recent ASKAP scheduling blocks, this is
generated in a self-consistent manner using the command-line
interfaces to the scheduling block and footprint services. This uses
the recorded beam footprint information in the scheduling block parset
to regenerate the central location of each beam.

The services are hosted at the MRO, and in the event of an
interruption to service, the use of these can be bypassed by setting
``USE_CLI=false``. This requires, however, that you have
previously-made metadata files available in the metadata directory (in
particular the footprint file).

Recent observations rely on the **src%d.footprint.rotation** parameter
in the scheduling block parset to determine the reference angle from
which the **pol_axis** parameter gives a field-based rotation. Some
scheduling blocks make use of this parameter yet do not report it in
the parset. If this is the case, you can manually specify the
reference value via the ``FOOTPRINT_PA_REFERENCE`` parameter in the
configuration file. The scheduling block parset value always takes
precendence. 

If you are processing an older SBID, for which the footprint
information is not available in the scheduling block, you must specify
the footprint information yourself using the parameters given
below. It is possible the footprint service does not know about your
footprint in this case - we then fall back to using the ACES script
*footprint.py*, which can accept a broader range of footprint names.

For re-processing of BETA data, you will need to set the ``IS_BETA``
parameter (so that the pipeline does not try to access the scheduling
block service), and provide the beam information using the parameters
below. 

It may be that you are using a non-standard arrangement that
footprint.py doesn’t cover. In this case you need to directly specify
the ``LINMOS_BEAM_OFFSETS`` variable (this is normally set by the
beamArrangements.sh script). Here is an example of how you would do
this (this is for the “diamond” footprint, band 1, PA=0)::
  
  LINMOS_BEAM_OFFSETS="linmos.feeds.beam0 = [-0.000,  0.000]
  linmos.feeds.beam1 = [-0.000,  1.244]
  linmos.feeds.beam2 = [-1.077,  0.622]
  linmos.feeds.beam3 = [-1.077, -0.622]
  linmos.feeds.beam4 = [-0.000, -1.244]
  linmos.feeds.beam5 = [ 1.077, -0.622]
  linmos.feeds.beam6 = [ 1.077,  0.622]
  linmos.feeds.beam7 = [-2.154,  0.000]
  linmos.feeds.beam8 = [ 2.154, -0.000]"

Note that ``LINMOS_BEAM_OFFSETS`` is only needed when all beams have
the same centre position (ie. when
``IMAGE_AT_BEAM_CENTRES=false``). If this is not the case, each beam
image has a different centre, and linmos simply uses that, putting a
primary beam at the centre of each image.

+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| Variable                           | Default                            | Parset equivalent       | Description                                                  |
+====================================+====================================+=========================+==============================================================+
| ``DO_MOSAIC``                      | true                               | none                    | Whether to mosaic the individual beam images, forming a      |
|                                    |                                    |                         | single, primary-beam-corrected image.                        |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``DO_MOSAIC_FIELDS``               | true                               | none                    | Whether to mosaic the different fields together (for when    |
|                                    |                                    |                         | there is more than one in the MS). If set to false, only the |
|                                    |                                    |                         | field-based mosaicking will be done.                         |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``JOB_TIME_LINMOS``                | ``JOB_TIME_DEFAULT`` (12:00:00)    | none                    | Time request for mosaicking                                  |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``NUM_CPUS_CONTCUBE_LINMOS``       | ""                                 | none                    | Total number of cores used for each of the spectral-line     |
|                                    |                                    |                         | mosaicking jobs. If blank (the default), the number used is  |
|                                    |                                    |                         | equal to the smaller of the number of cores used for the     |
|                                    |                                    |                         | continuum cube imaging (``NUM_CPUS_CONTCUBE_SCI``) or the    |
|                                    |                                    |                         | number of continuum channels.                                |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``NCHAN_PER_CORE_SPECTRAL_LINMOS`` | 8                                  | none                    | Number of channels to be handled at once by a single process |
|                                    |                                    |                         | in the spectral-line mosaicking. This should divide evenly   |
|                                    |                                    |                         | into the number of spectral channels. This will determine the|
|                                    |                                    |                         | number of cores assigned to the spectral-line mosaicking,    |
|                                    |                                    |                         | unless ``NUM_CPUS_SPECTRAL_LINMOS`` is provided.             |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``NUM_CPUS_SPECTRAL_LINMOS``       | ""                                 | none                    | Total number of cores used for each of the spectral-line     |
|                                    |                                    |                         | mosaicking jobs. If blank, the number used is deterined by   |
|                                    |                                    |                         | the total number of channels and                             |
|                                    |                                    |                         | ``NCHAN_PER_CORE_SPECTRAL_LINMOS``.                          |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``MOSAIC_SELFCAL_LOOPS``           | false                              | none                    | Whether to make mosaics of each self-calibration loop.       |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``FOOTPRINT_PA_REFERENCE``         | ""                                 | none                    | The reference rotation angle for the footprint. This should  |
|                                    |                                    |                         | only be given if the scheduling block parset does not have   |
|                                    |                                    |                         | the **common.src.%d.footprint.rotation** parameter, or if    |
|                                    |                                    |                         | you want to over-ride that value. If not given, the          |
|                                    |                                    |                         | footprint.rotation value will be used, or (in its absence),  |
|                                    |                                    |                         | zero.                                                        |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``BEAM_FOOTPRINT_NAME``            | diamond                            | none                    | The name of the beam footprint. This needs to be recognised  |
|                                    |                                    |                         | by the ACES tool *footprint.py*, which generates the offsets |
|                                    |                                    |                         | required by the linmos application.                          |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``BEAM_FOOTPRINT_PA``              | 0                                  | none                    | The position angle of the beam footprint pattern. Passed to  |
|                                    |                                    |                         | footprint.py.                                                |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``BEAM_PITCH``                     | 1.24                               | none                    | The pitch, or beam spacing, in degrees. Passed to            |
|                                    |                                    |                         | footprint.py.                                                |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``FREQ_BAND_NUMBER``               | ""                                 | none                    | Which frequency band are we in - determines beam arrangement |
|                                    |                                    |                         | (1,2,3,4). Passed to footprint.py. If not given, the pitch   |
|                                    |                                    |                         | value is used to set the beam separation. The band is        |
|                                    |                                    |                         | overridden by the pitch as well.                             |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``NUM_BEAMS_FOOTPRINT``            | 36                                 | none                    | The number of beams in the footprint. In regular operation,  |
|                                    |                                    |                         | this will be determined from the footprint service, but will |
|                                    |                                    |                         | need to be specified in the case of non-standard or BETA     |
|                                    |                                    |                         | footprints.                                                  |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``LINMOS_BEAM_OFFSETS``            | no default                         | feeds.beam{i}           | Parset entries that specify the beam offsets for use by      |
|                                    |                                    | (:doc:`../calim/linmos`)| linmos. Needs to have one entry for each beam being          |
|                                    |                                    |                         | mosaicked. See above for an example. Only provide this if    |
|                                    |                                    |                         | running footprint.py is not going to give you what you want  |
|                                    |                                    |                         | (eg. non-standard beam locations).                           |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``LINMOS_BEAM_SPACING``            | "1deg"                             | feeds.spacing           | Scale factor for beam arrangement, in format like ‘1deg’.    |
|                                    |                                    | (:doc:`../calim/linmos`)| This should not be altered if you are using a standard       |
|                                    |                                    |                         | footprint from footprint.py (ie. with                        |
|                                    |                                    |                         | ``BEAM_FOOTPRINT_NAME``).                                    |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``LINMOS_CUTOFF``                  | 0.2                                | linmos.cutoff           | The primary beam cutoff, as a fraction of the peak           |
|                                    |                                    | (:doc:`../calim/linmos`)|                                                              |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
| ``LINMOS_PSF_REF``                 | 0                                  | psfref                  | Reference beam for PSF (0-based) - which beam to take the    |
|                                    |                                    | (:doc:`../calim/linmos`)| PSF information from.                                        |
+------------------------------------+------------------------------------+-------------------------+--------------------------------------------------------------+
 
