msmerge (Measurement Merging Utility)
=====================================

The *msmerge* utility is used to merge measurement sets in frequency.

The intended use-cases of this tool are:

- Merge measurement sets from different frequency sub-bands into a single
  measurement set. It is used, for example, to create a single simulated
  measurement set when one needs to split a simulation into sub-bands or
  to combine the channel ranges for a single beam from the sub-band single 
  beam files produced by ingest.

It is assumed all input files have the same number of channels and are listed in the correct order to combine them into the full spectrum output.

Running the program
-------------------

It can be run with the following command ::

   $ msmerge -o output_file list_of_input_files

The *msmerge* program is not parallel/distributed, it runs in a single process.

Configuration Parameters
------------------------

At this time *msmerge* does not accept a configuration parameter file.
However it has a number of command line flags ::

   -x <n> : specify the output tile size for correlations/polarisations
   -c <n> : specify the output tile size for channels
   -r <n> : specify the output tile size for rows
   -o <output file> : specify the output MeasurementSet
   -i <input files> : specify the input files (or just list them at the end)

The default tiling is 4 polarizations, 1 channel, and as many rows as can be
cached efficiently.


Example
-------

**Example 1:**

.. code-block:: bash

   $ msmerge -o fullband.ms subband_???.ms

**Example 2:**

.. code-block:: bash

   $ msmerge -x 4 -c 54 -o output.ms channel1.ms channel2.ms channel3.ms

