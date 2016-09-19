FITS conversion
===============

The task *imageToFITS* task provides a mechanism for converting a
CASA-format image to a FITS file. It allows a lot more flexibility
than the casacore *image2fits* task, providing a front-end to the
ImageFITSConverter::ImageToFITS function in casacore. Each of the
arguments for this function are exposed via the ParameterSet
interface.

The task also allows the user to set image header/metadata values and
history statements. These are applied to the CASA image first, and
then flow through into the FITS file.

Running the program
-------------------

It can be run with the following command, where "config.in" is a file
containing the configuration parameters described in the next
section. ::

   $  imageToFITS -c config.in

The *imageToFITS* program is not parallel/distributed, it runs in a
single process operating on a single input CASA image.

Configuration Parameters
------------------------

All parameters should be prefaced by **ImageToFITS.**

+----------------------+----------------+------------+---------------------------------------------------------------------+
|*Parameter*           |*Type*          |*Default*   |*Description*                                                        |
+======================+================+============+=====================================================================+
|**Metadata**          |                |            |                                                                     |
+----------------------+----------------+------------+---------------------------------------------------------------------+
| headers              | vector<string> | []         |Vector of strings, being metadata header names to add to the         |
|                      |                |            |image. Each header name is then specified as its own parameter to    |
|                      |                |            |provide the value.                                                   |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|headers. *headerName* | string         |*None*      |Value for the metadata header *headerName*                           |
+----------------------+----------------+------------+---------------------------------------------------------------------+
| history              | vector<string> | []         |Vector of strings to be recorded in the image metadata as HISTORY    |
|                      |                |            |statements. Each string gets its own HEADER line.                    |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|**FITS conversion**   |                |            |                                                                     |
+----------------------+----------------+------------+---------------------------------------------------------------------+
| casaimage            |string          |*None*      |The input CASA-format image.                                         |
+----------------------+----------------+------------+---------------------------------------------------------------------+
| fitsimage            |string          |*None*      |The output FITS-format image.                                        |
+----------------------+----------------+------------+---------------------------------------------------------------------+
| memoryInMB           |int             | 64         |Setting this to zero will result in row-by-row copying, otherwise it |
|                      |                |            |will attempt to copy with as large a chunk-size as possible, while   |
|                      |                |            |fitting in the desired memory.                                       |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|preferVelocity        |bool            |false       |Write a velocity primary spectral axis if possible.                  |
|                      |                |            |                                                                     |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|opticalVelocity       |bool            |true        |If writing a velocity, use the optical definition (otherwise use     |
|                      |                |            |radio).                                                              |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|bitpix                |float           |-32         |*bitpix* can presently be set to -32 or 16 only.                     |
|                      |                |            |                                                                     |
|                      |                |            |When *bitpix* is 16 it will write BSCALE and BZERO into the FITS     |
|                      |                |            |file. If *minPix* is greater than *maxPix* the minimum and maximum   |
|                      |                |            |pixel values will be determined from the array, otherwise the        |
|                      |                |            |supplied values will be used and pixels outside that range will be   |
|                      |                |            |truncated to the minimum and maximum pixel values.                   |
|                      |                |            |                                                                     |
|                      |                |            |Note that this truncation does not occur for *bitpix=-32*.           |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|minpix                |float           |1.0         |Minimum pixel value for scaling in *bitpix=16* case.                 |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|maxpix                |float           |-1.0        |Minimum pixel value for scaling in *bitpix=16* case.                 |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|allowOverwrite        |bool            |false       |If true, allow *fitsimage* to be overwritten if it already exists.   |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|degenerateLast        |bool            |false       |If true, axes of length 1 will be written last to the header.        |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|stokesLast            |bool            |false       |Put the Stokes axis last in the FITS file.                           |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|verbose               |bool            |true        |More log messages.                                                   |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|preferWavelength      |bool            |false       |If true, write a wavelength primary axis.                            |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|airWavelength         |bool            |false       |If true and *preferWavelength* is true write an air wavelength       |
|                      |                |            |primary axis.                                                        |
+----------------------+----------------+------------+---------------------------------------------------------------------+
|copyHistory           |bool            |true        |Whether to copy the CASA image's history                             |
+----------------------+----------------+------------+---------------------------------------------------------------------+


Example
-------

.. code-block:: bash

   # CASA image input
   ImageToFITS.casaimage = image.i.cube.linmos.restored
   # FITS image output
   ImageToFITS.fitsimage = image.i.cube.linmos.restored.fits
   # Put the Stokes axis last
   ImageToFITS.stokesLast = true
   # Ensure the spectral axis stays in frequency, as we are regularly sampled there.
   ImageToFITS.preferVelocity = false
   # Add some additional headers
   ImageToFITS.headers = ["project","sbid","date-obs","duration"]
   ImageToFITS.headers.project = AS033
   ImageToFITS.headers.sbid = 1531
   ImageToFITS.headers.date-obs = 2016-06-29T11:21:15.9
   ImageToFITS.headers.duration = 21534.3
   # Add some history statements showing provenance
   ImageToFITS.history = ["Produced with ASKAPsoft version 0.14.0","Produced using ASKAP pipeline version 0.14.0"]
