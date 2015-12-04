User Parameters - Mosaicking Continuum images
=============================================

The linmos tool requires beam offsets to know where to place the
primary beam models. Most of the time, these will be generated using
the ACES *footprint.py* tool, and so all you need to specify are the
footprint name, the position angle and pitch (separation of beams),
and the frequency band you are observing in.

Note that, to use *footprint.py*, you will need to load the ACES
module (to get the correct python packages) and have an up-to-date
version of the ACES subversion repository. If you have not loaded the
ACES module, it is likely the *footprint.py* task will fail, and
mosaicking will be disabled.

It may be that you are using a non-standard arrangement that
footprint.py doesn’t cover. In this case you need to directly specify
the ``LINMOS_BEAM_OFFSETS`` variable (this is normally set by the
beamArrangements.sh script). Here is an example of how you would do
this (this is for the “diamond” footprint, band 1, PA=0::
  
  LINMOS_BEAM_OFFSETS="linmos.feeds.beam0 = [-0.000,  0.000]
  linmos.feeds.beam1 = [-0.000,  1.244]
  linmos.feeds.beam2 = [-1.077,  0.622]
  linmos.feeds.beam3 = [-1.077, -0.622]
  linmos.feeds.beam4 = [-0.000, -1.244]
  linmos.feeds.beam5 = [ 1.077, -0.622]
  linmos.feeds.beam6 = [ 1.077,  0.622]
  linmos.feeds.beam7 = [-2.154,  0.000]
  linmos.feeds.beam8 = [ 2.154, -0.000]"

+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| Variable                | Default     | Parset equivalent       | Description                                                 |
+=========================+=============+=========================+=============================================================+
| ``DO_MOSAIC``           | true        | none                    | Whether to mosaic the individual beam images, forming a     |
|                         |             |                         | single, primary-beam-corrected image.                       |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``BEAM_FOOTPRINT_NAME`` | diamond     | none                    | The name of the beam footprint. This needs to be recognised |
|                         |             |                         | by the ACES tool *footprint.py*, which generates the offsets|
|                         |             |                         | required by the linmos application.                         |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``BEAM_FOOTPRINT_PA``   | 0           | none                    | The position angle of the beam footprint pattern. Passed to |
|                         |             |                         | footprint.py.                                               |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``BEAM_PITCH``          | 1.24        | none                    | The pitch, or beam spacing, in degrees. Passed to           |
|                         |             |                         | footprint.py.                                               |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``FREQ_BAND_NUMBER``    | 1           | none                    | Which frequency band are we in - determines beam arrangement|
|                         |             |                         | (1,2,3,4). Passed to footprint.py.                          |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``LINMOS_BEAM_OFFSETS`` | no default  | feeds.beam{i}           | Parset entries that specify the beam offsets for use by     |
|                         |             | (:doc:`../calim/linmos`)| linmos. Needs to have one entry for each beam being         |
|                         |             |                         | mosaicked. See above for an example. Only provide this if   |
|                         |             |                         | running footprint.py is not going to give you what you want |
|                         |             |                         | (eg. non-standard beam locations).                          |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``LINMOS_BEAM_SPACING`` | "1deg"      | feeds.spacing           | Scale factor for beam arrangement, in format like ‘1deg’.   |
|                         |             | (:doc:`../calim/linmos`)| This should not be altered if you are using a standard      |
|                         |             |                         | footprint from footprint.py (ie. with                       |
|                         |             |                         | ``BEAM_FOOTPRINT_NAME``).                                   |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
| ``LINMOS_PSF_REF``      |0            | psfref                  | Reference beam for PSF (0-based) - which beam to take the   |
|                         |             | (:doc:`../calim/linmos`)| PSF information from.                                       |
+-------------------------+-------------+-------------------------+-------------------------------------------------------------+
 
