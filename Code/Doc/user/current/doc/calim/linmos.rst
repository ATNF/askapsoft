linmos (Linear Mosaic Applicator)
=================================

This page provides instruction for using the linmos program. The purpose of
this software is to perform a linear mosaic of a set of images.

Running the program
-------------------

It can be run with the following command, where "config.in" is a file containing
the configuration parameters described in the next section. ::

   $ linmos -c config.in

The *linmos* program is not parallel/distributed.

Parallel linmos (*linmos-mpi*)
------------------------------

There is a parallel version of *linmos* which will divide the mosaic over the number of
ranks. This improves the run time markedly as the I/O is distributed. Furthermore this also
reduces the memory load.

Configuration Parameters
------------------------

The following table contains the configuration parameters to be specified in the *config.in*
file shown on above command line. Note that each parameter must be prefixed with "linmos.".
For example, the *weighttype* parameter becomes *linmos.weighttype*.

.. note:: During the BETA campaign there is no default option for weighttype. This option must
          be set.

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|names             |vector<string>    |*none*        |Names of the input images. If these images start with       |
|                  |                  |              |"image" and have associated sensitivity images, the latter  |
|                  |                  |              |are integrated into a sensitivity image for the mosaic.     |
+------------------+------------------+--------------+------------------------------------------------------------+
|weights           |vector<string>    |null          |Optional parameter (required if using weight images). Names |
|                  |                  |              |of input images containing pixel weights. There must be one |
|                  |                  |              |weight image for each image, and the size must match.       |
|                  |                  |              |Ignored if *weighttype=FromPrimaryBeamModel* or if          |
|                  |                  |              |*findmosaics=true*.                                         |
+------------------+------------------+--------------+------------------------------------------------------------+
|outname           |string            |*none*        |Name of the output image. Ignored if *findmosaics=true*.    |
+------------------+------------------+--------------+------------------------------------------------------------+
|outweight         |string            |*none*        |Name of output image containing pixel weights. Ignored if   |
|                  |                  |              |*findmosaics=true*.                                         |
+------------------+------------------+--------------+------------------------------------------------------------+
|weighttype        |string            |*none*        |How to determine the pixel weights. Options:                |
|                  |                  |              |                                                            |
|                  |                  |              |- **FromWeightImages**: from weight images. Parameter       |
|                  |                  |              |  *weights* must be present and there must be a one-to-one  |
|                  |                  |              |  correspondence with the input images.                     |
|                  |                  |              |- **FromPrimaryBeamModel**: using a Gaussian primary-beam   |
|                  |                  |              |  model. **If beam centres are not specified (see below),   |
|                  |                  |              |  the reference pixel of each input image is used.**        |
|                  |                  |              |- **Combined**: ** linmos-mpi only ** uses both the weight  |
|                  |                  |              |  images and the PB model to form the pixel weight          |
+------------------+------------------+--------------+------------------------------------------------------------+
|weightstate       |string            |Corrected     |The weighting state of the input images.                    |
|                  |                  |              |Options:                                                    |
|                  |                  |              |                                                            |
|                  |                  |              |- **Corrected**: Direction-dependent beams/weights have     |
|                  |                  |              |  been divided out of input images.                         |
|                  |                  |              |- **Inherent**: Input images retain the natural             |
|                  |                  |              |  primary-beam weighting of the visibilities.               |
|                  |                  |              |- **Weighted**: Full primary-beam-squared weighting.        |
+------------------+------------------+--------------+------------------------------------------------------------+
|cutoff            |float             |0.01          |Desired cutoff of the gain function used to form weights,   |
|                  |                  |              |relative to the maximum gain.                               |
+------------------+------------------+--------------+------------------------------------------------------------+
|psfref            |uint              |0             |Which of the input images to extract restoring-beam         |
|                  |                  |              |information from. The default behaviour is to use the       |
|                  |                  |              |first image specified (indices start at 0).                 |
+------------------+------------------+--------------+------------------------------------------------------------+
|nterms            |uint              |-1            |Process multiple taylor-term images. The string "taylor.0"  |
|                  |                  |              |must be present in both input and output image names        |
|                  |                  |              |(including weights images), and it will be incremented from |
|                  |                  |              |0 to nterms-1. Ignored if *findmosaics=true.*               |
+------------------+------------------+--------------+------------------------------------------------------------+
|findmosaics       |bool              |false         |Instead of specifying specific input and output files to    |
|                  |                  |              |mosaic, search the current directory for suitable mosaics.  |
|                  |                  |              |Parameter *names* is used to specify a vector of tags, and  |
|                  |                  |              |all groups of images that have names that are equal apart   |
|                  |                  |              |from these tags are mosaicked together. Groups must have one|
|                  |                  |              |image per tag. Currently only groups with prefixes of       |
|                  |                  |              |"image" and "residual" are allowed, with prefixes "weights" |
|                  |                  |              |and "sensitivity" special cases that are searched for once  |
|                  |                  |              |groups are identified. Parameters *weights*, *outname*,     |
|                  |                  |              |*outweight* and *nterms* are ignored if *findmosaic=true*.  |
+------------------+------------------+--------------+------------------------------------------------------------+

If input images need to be regridded, the following ImageRegrid options are available:

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|regrid.method     |string            |linear        |ImageRegrid interpolation method: *nearest*, *linear*,      |
|                  |                  |              |*cubic* or *lanczos*.                                       |
+------------------+------------------+--------------+------------------------------------------------------------+
|regrid.decimate   |uint              |3             |ImageRegrid decimation factor. In the range 3-10 is likely  |
|                  |                  |              |to provide the best performance/accuracy tradeoff           |
+------------------+------------------+--------------+------------------------------------------------------------+
|regrid.replicate  |bool              |false         |ImageRegrid *replicate* option.                             |
+------------------+------------------+--------------+------------------------------------------------------------+
|regrid.force      |bool              |false         |ImageRegrid *force* option.                                 |
+------------------+------------------+--------------+------------------------------------------------------------+

Definition of beam centres
--------------------------

If weights are generated from primary-beam models (*weighttype=FromPrimaryBeamModel*), it is possible to set the
beam centres from within the parset. Since this is most likely useful when each input image comes from a different
multi-beam feed, *feeds* offset parameters from other applications are used for this. If the origin of the *beams*
offset system is not specified, using either *feeds.centre* or *feeds.centreref*, any offsets are ignored and the
reference pixel of each input image is used as the primary-beam centre.

The *feeds* parameters can be given either in the main linmos parset or a separate offsets parset file set by the
*feeds.offsetsfile* parameter.

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|feeds.centre      |vector<string>    |*none*        |Optional parameter (it or *feeds.centreref* required when   |
|                  |                  |              |specifying beam offsets).                                   |
|                  |                  |              |Two-element vector containing the right ascension and       |
|                  |                  |              |declination that all of the offsets are relative to.        |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.centreref   |int               |*none*        |Optional parameter (it or *feeds.centre* required when      |
|                  |                  |              |specifying beam offsets). Which of the input images to use  |
|                  |                  |              |to automatically set *feeds.centre*. Indices start at 0.    |
|                  |                  |              |If neither of these parameters are set, the reference pixel |
|                  |                  |              |of each input image is used as the primary-beam centre.     |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.spacing     |string            |*none*        |Optional parameter (required when specifying beam offsets   |
|                  |                  |              |in the main linmos parset). Beam/feed spacing when giving   |
|                  |                  |              |offsets in the main linmos parset. If *feeds.offsetsfile*   |
|                  |                  |              |is given, this parameter will be ignored.                   |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.names[i]    |vector<string>    |*none*        |Optional parameter (required when specifying beam offsets   |
|(one per input    |                  |              |in the main linmos parset). Two-element vector containing   |
|image)            |                  |              |the beam offset relative to the *feeds.centre* parameter.   |
|                  |                  |              |Offsets correspond to hour angle and declination.           |
|                  |                  |              |*names[i]* should match the names of the input images,      |
|                  |                  |              |given in *linmos.names* (see above). If *feeds.offsetsfile* |
|                  |                  |              |is given, these parameters will be ignored.                 |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.offsetsfile |string            |*none*        |Optional parameter. Name of the optional beam/feed offsets  |
|                  |                  |              |parset. If present, any offsets specified in the main       |
|                  |                  |              |linmos parset will be ignored.                              |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.names       |vector<string>    |*none*        |Optional parameter (required either here or below when      |
|                  |                  |              |specifying a beam offsets parset). The beam offsets parset  |
|                  |                  |              |should have one line per input image, with parameter keys   |
|                  |                  |              |(minus the *feeds.* prefix) specified by this parameter. If |
|                  |                  |              |the offsets parset also contains a *names* parameter, the   |
|                  |                  |              |main linmos entry will hold, to allow a subset of beams     |
|                  |                  |              |from a general to be chosen.                                |
+------------------+------------------+--------------+------------------------------------------------------------+

If feed offsets are provided via an additional parset (i.e. not that one passed directly to
the linmos program), the file shall have the following format:

.. note:: These parameters, specified in the external file, do not require the "limos." prefix.

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|feeds.names       |vector<string>    |null          |Optional parameter (required either here or above when      |
|                  |                  |              |specifying a beam offsets parset). The beam offsets parset  |
|                  |                  |              |should have one line per input image, with parameter keys   |
|                  |                  |              |(minus the *feeds.* prefix) specified by this parameter. If |
|                  |                  |              |the offsets parset also contains a *names* parameter, the   |
|                  |                  |              |main linmos entry will hold, to allow a subset of beams     |
|                  |                  |              |from a general to be chosen.                                |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.spacing     |string            |*none*        |Beam/feed spacing. When using this extra offsets parset,    |
|                  |                  |              |the spacing needs to be specified in this parset.           |
+------------------+------------------+--------------+------------------------------------------------------------+
|feeds.beamnames[i]|vector<string>    |*none*        |Two-element vector containing the beam offset relative to   |
|(one per input    |                  |              |the *feeds.centre* parameter. Offsets correspond to hour    |
|image)            |                  |              |angle and declination. *beamnames[i]* should match the      |
|                  |                  |              |names given in feeds.names* (see above).                    |
+------------------+------------------+--------------+------------------------------------------------------------+


Alternate Primary Beam Models
-----------------------------

It is possible to select the model that is used for the weighting. This is selected in the linmos parset by
the key "primarybeam"

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|primarybeam       |string            |"GaussianPB"  |Optional parameter that allows the user to select which     |
|                  |                  |              |primary beam will be used in weighting. The parameters of   |
|                  |                  |              |which can also be altered if required. Also supported are   |
|                  |                  |              |MWA primary beams, via primarybeam = MWA_PB.                |
+------------------+------------------+--------------+------------------------------------------------------------+

**Gaussian Primary Beam Options**

You can choose the aperture size and scaling parameters both of the FWHM of the beam and a scaling of the exponent.
In the parfile these are sub parameters of the Primary beam type. (e.g linmos.primarybeam.GaussianPB.aperture)

The default Gaussian Primary beam is now 2 dimensional. But unless the user specifies x and w widths they just get the symmetric beam as defined by the aperture.

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|aperture          |double            |12            |Aperture size in metres.                                    |
+------------------+------------------+--------------+------------------------------------------------------------+
|fwhmscaling       |double            |1.09          |Scaling of the full width half max of the Gaussian          |
+------------------+------------------+--------------+------------------------------------------------------------+
|expscaling        |double            | 4 log(2)     |Scaling of the primary beam exponent                        |
+------------------+------------------+--------------+------------------------------------------------------------+

The 2 dimensional beam is governed by the following parameters.

+------------------+------------------+--------------+------------------------------------------------------------+
|**2D-Parameters** |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
| (x/y)width       |double            | 0.0          |Angular width in rad. of the x (N-S) and y (E-W) Gaussian   |
+------------------+------------------+--------------+------------------------------------------------------------+
| (x/y)off         |double            |0.0           |Angular offset from nominal beamcentre in rad., E, N are +ve|
+------------------+------------------+--------------+------------------------------------------------------------+
| alpha            |double            |0.0           |PA in rad. measured from North in an +ve RA direction       |
+------------------+------------------+--------------+------------------------------------------------------------+


**MWA Primary Beam Options**

+-------------------+------------------+-----------------+------------------------------------------------------------+
|**Parameter**      |**Type**          |**Default**      |**Description**                                             |
+===================+==================+=================+============================================================+
| latitude          |double            | -26.703319 deg  | Array latitude in radians                                  |
+-------------------+------------------+-----------------+------------------------------------------------------------+
| longitude         |double            | 116.67081 deg   | Array longitude in radians                                 |
+-------------------+------------------+-----------------+------------------------------------------------------------+
| dipole.separation |double            | 1.10 metres     | Dipole separation                                          |
+-------------------+------------------+-----------------+------------------------------------------------------------+
| dipole.height     |double            | 0.30 metres     | dipole hheight                                             |
+-------------------+------------------+-----------------+------------------------------------------------------------+

Primary Beam Corrections to the Taylor terms
--------------------------------------------

The primary beam is a function of frequency. Therefore the apparent spectral index of a point source away from beam centre
will contain a contribution from the frequency dependence of the primary beam. It is possible to estimate this contribution
and remove it by scaling the Taylor term images appropriately.

.. note:: This is an analytic correction assuming a symmetric Gaussian beam

+------------------+------------------+--------------+------------------------------------------------------------+
|**Parameter**     |**Type**          |**Default**   |**Description**                                             |
+==================+==================+==============+============================================================+
|removebeam        |bool              |false         |Remove beam from the Taylor term images                     |
+------------------+------------------+--------------+------------------------------------------------------------+

Examples
--------

**Example 1:**

Example linmos parset to combine individual feed images from a 36-feed simulation.  Weights
images are used to weight the pixels.

.. code-block:: bash

    linmos.weighttype = FromWeightImages

    linmos.names      = [image_feed00..35_offset.i.dirty.restored]
    linmos.weights    = [weights_feed00..35_offset.i.dirty]

    linmos.outname    = image_mosaic.i.dirty.restored
    linmos.outweight  = weights_mosaic.i.dirty


**Example 2:**

Example linmos parset to combine the four inner-most feed images from a 36-feed observation.
Gaussian primary-beam models are used to weight the pixels. The primary-beam offsets are
provided in an external file.

.. code-block:: bash

    linmos.weighttype       = FromPrimaryBeamModel

    linmos.names            = [image_feed14..15.i.dirty.restored, image_feed20..21.i.dirty.restored]

    linmos.outname          = image_mosaic.i.dirty.restored
    linmos.outweight        = weights_mosaic.i.dirty

    linmos.feeds.centre     = [12h30m00.00, -45.00.00.00]

    # specify a beam offsets file
    linmos.feeds.offsetsfile = linmos_beam_offsets.in

    # Specify which feeds from the "offsetsfile" (specified above) are to be used
    linmos.feeds.names       = [PAF36.feed14..15, PAF36.feed20..21]

Below is the *linmos_beam_offsets.in* file refered to in the above parameter set:

.. code-block:: bash

    feeds.spacing            = 1deg
    <snip>
    feeds.PAF36.feed14       = [-0.5, -0.5]
    feeds.PAF36.feed15       = [-0.5,  0.5]
    <snip>
    feeds.PAF36.feed20       = [0.5, -0.5]
    feeds.PAF36.feed21       = [0.5,  0.5]
    <snip>


**Example 3:**

Example linmos parset to combine the four inner-most feed images from a 36-feed simulation.
The primary-beam offsets directly in the parameter set.

.. code-block:: bash

    linmos.weighttype       = FromPrimaryBeamModel

    linmos.names            = [image_feed14..15.i.dirty.restored, image_feed20..21.i.dirty.restored]

    linmos.outname          = image_mosaic.i.dirty.restored
    linmos.outweight        = weights_mosaic.i.dirty

    linmos.feeds.centre     = [12h30m00.00, -45.00.00.00]

    linmos.feeds.spacing    = 1deg
    linmos.feeds.image_feed14.i.dirty.restored = [-0.5, -0.5]
    linmos.feeds.image_feed15.i.dirty.restored = [-0.5,  0.5]
    linmos.feeds.image_feed20.i.dirty.restored = [0.5, -0.5]
    linmos.feeds.image_feed21.i.dirty.restored = [0.5,  0.5]


**Example 4:**

Example linmos parset to combine individual feed images from a 36-feed simulation for each of three
separate taylor terms 0, 1 and 2. The location of taylor.* in all inputs and outputs is given explicitly.

.. code-block:: bash

    linmos.weighttype = FromWeightImages

    linmos.names      = [image_feed00..35_offset.i.dirty.taylor.0.restored]
    linmos.weights    = [weights_feed00..35_offset.i.dirty.taylor.0]

    linmos.outname    = image_mosaic.i.dirty.taylor.0.restored
    linmos.outweight  = weights_mosaic.i.dirty.taylor.0

    linmos.nterms = 3


**Example 5:**

Example linmos parset to combine individual feed images from a 36-feed simulation. A mosaics is made for each set
of 36 images that has one image for each tag (param "names") but filenames that are otherwise the same. Only the
"image" and "residual" prefixes are currently supported. For example, if the outputs produced for Data Challenge 1A
were produced for each feed and stored in a single directory, the following mosaics would be made:
image_linmos.i.clean.taylor.0, image_linmos.i.clean.taylor.0.restored, image_linmos.i.clean.taylor.1,
image_linmos.i.clean.taylor.1.restored, image_linmos.i.dirty.restored, residual_linmos.i.clean.taylor.0 and
residual_linmos.i.clean.taylor.1. Associated weights and sensitivity images would also be made, however in
situations where multiple mosaics have the same weights or sensitivites (e.g. image_linmos.i.clean.taylor.0,
image_linmos.i.clean.taylor.0.restored and residual_linmos.i.clean.taylor.0), only one would be made.

Furthermore, since the DC1A does not seem to produce weights.*.taylor.2 and we have specified weighttype
FromWeightImages, mosaic image_linmos.clean.taylor.2 would not be made. It would be produced if weighttype were
FromPrimaryBeamModel.

.. code-block:: bash

    linmos.weighttype  = FromWeightImages
    linmos.findmosaics = true
    linmos.names       = [feed00..35_offset]
