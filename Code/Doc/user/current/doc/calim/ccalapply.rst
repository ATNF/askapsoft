ccalapply (Calibration Applicator)
==================================

This page provides instruction for using the ccalapply program. The purpose of
this software is to apply calibration parameters to Measurement Sets.

Running the program
-------------------

It can be run with the following command, where "config.in" is a file containing
the configuration parameters described in the next section. ::

   $ ccalapply -c config.in

The *ccalapply* program can either be used in the serial mode when it runs on a single input measurement set or
in parallel/distributed mode where there are two options (see the *distribute* parset parameter). First, each
rank can work with its own measurement set. This may be handy if one has one measurement set per beam or other
distribution which leaves the frequency axis intact. The second option involves distribution of data in frequency
space internally. Only one measurement set should be given in this case and the appropriate split is done based on
the available number of ranks. This is the default behaviour. These details have no effect in the serial mode.

Configuration Parameters
------------------------

The following table contains the configuration parameters to be specified in the "config.in"
file shown on above command line. Note that each parameter must be prefixed with "Ccalapply.".
For example, the "dataset" parameter becomes "Ccalapply.dataset".

In addition to the below parameters, those described in :doc:`calibration_solutions`
are applicable. Specifically:

* Ccalapply.calibaccess
* Ccalapply.calibaccess.parset (if calibaccess is "parset"); or
* Ccalapply.calibaccess.table (if calibaccess is "table")

+--------------------------+------------------+--------------+----------------------------------------------------+
|**Parameter**             |**Type**          |**Default**   |**Description**                                     |
+==========================+==================+==============+====================================================+
|dataset                   |string            |None          |The name of the measurement set to which the        |
|                          |                  |              |calibration parameters will be applied. In the      |
|                          |                  |              |parallel mode with the *distribution* option is     |
|                          |                  |              |false, usual substitute rules apply.                |
+--------------------------+------------------+--------------+----------------------------------------------------+
|calibrate.scalenoise      |bool              |false         |If true, the noise estimate will be scaled in       |
|                          |                  |              |accordance with the applied calibrator factor to    |
|                          |                  |              |achieve proper weighting.                           |
+--------------------------+------------------+--------------+----------------------------------------------------+
|calibrate.allowflag       |bool              |false         |If true, corresponding visibilities are flagged if  |
|                          |                  |              |the inversion of Mueller matrix fails. Otherwise, an|
|                          |                  |              |exception is thrown should the matrix inversion fail|
+--------------------------+------------------+--------------+----------------------------------------------------+
|calibrate.ignorebeam      |bool              |false         |If true, the calibration solution corresponding to  |
|                          |                  |              |beam 0 will be applied to all beams                 |
+--------------------------+------------------+--------------+----------------------------------------------------+
|calibrate.ignorechannel   |bool              |false         |If true, the same calibration solution will be      |
|                          |                  |              |applied to all channels. Use this to speed up the   |
|                          |                  |              |application of selfcal gains.                       |
+--------------------------+------------------+--------------+----------------------------------------------------+
|freqframe                 |string            |topo          |Frequency frame to work in (the frame is converted  |
|                          |                  |              |when the dataset is read). Either lsrk or topo is   |
|                          |                  |              |supported.                                          |
+--------------------------+------------------+--------------+----------------------------------------------------+
|maxchunkrows              |unsigned int      |max integer   |If defined, the chunk size presented at each iterat\|
|                          |                  |              |ion will be restricted to have at most this number  |
|                          |                  |              |of rows. It doesn't affect the result, but may give |
|                          |                  |              |different performance in the case of long spectral  |
|                          |                  |              |axis (think of it as a slice in rows will be taken  |
|                          |                  |              |once and then each spectral slice will be made from |
|                          |                  |              |this reduced cube for the price of having more      |
|                          |                  |              |iterations).                                        |
+--------------------------+------------------+--------------+----------------------------------------------------+
|distribute                |bool              |true          |If the application is executed in the parallel mode |
|                          |                  |              |(i.e. more than one rank is available) and this     |
|                          |                  |              |option is true, the data are split in frequency     |
|                          |                  |              |between all available workers (i.e. ranks with non-\|
|                          |                  |              |zero numbers) which do calibration application. The |
|                          |                  |              |master rank (always rank 0) writes the result to the|
|                          |                  |              |measurement set. If this option is false, each rank |
|                          |                  |              |is expected to deal with its own measurement set and|
|                          |                  |              |its own reading, calibration application and        |
|                          |                  |              |writing. Note, there is no master in this mode. So  |
|                          |                  |              |the substitution rules should use rank (%r) rather  |
|                          |                  |              |than the worker (%w) number. This option has no     |
|                          |                  |              |effect in the serial mode.                          |
+--------------------------+------------------+--------------+----------------------------------------------------+

Example
-------

.. code-block:: bash

    Ccalapply.dataset                   = mydataset.ms

    Ccalapply.calibaccess               = table
    Ccalapply.calibaccess.table         = calparameters.tab
