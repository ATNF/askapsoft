mslist (Measurement summary and data inspection utility)
========================================================

The *mslist* utility is a simple, command-line driven measurement set inspection tool.
It uses casacore tasks *MSSummary* and *MSLister*.

See documentation on casacore tasks for more information.

Running the program
-------------------

It can be run with the following command ::

   $ mslist [options] MS_filename

The *mslist* program is not parallel/distributed, it runs in a single process.

Casacore tasks *MSSummary* and *MSLister* write to stderr, so to redirect output to a
pipe or a file, first redirect stderr to stdout. For example, in bash: ::

   $ mslist [options] MS_filename > MS_summary.txt 2>&1

Configuration Parameters
------------------------

*mslist* does not accept a configuration parameter file, but does accept a
number of command-line options.

+----------------------+----------------------------------------------------------------------------------+
|**Parameter**         |**Description**                                                                   |
+======================+==================================================================================+
|``--brief``           |Brief listing (default, with verbose). Calls *MSSummary* function *listMain*.     |
+----------------------+----------------------------------------------------------------------------------+
|``--full``            |More extensive listing; where, what and how.                                      |
+----------------------+----------------------------------------------------------------------------------+
|``--what``            |What what observed? (fields, times, etc.) See *MSSummary* function *listWhat*.    |
+----------------------+----------------------------------------------------------------------------------+
|``--how``             |How was it observed? (antennas, frequencies, etc.) See *MSSummary* function       |
|                      |*listHow*.                                                                        |
+----------------------+----------------------------------------------------------------------------------+
|``--tables``          |List tables in the measurement set. See *MSSummary* function *listTables*.        |
+----------------------+----------------------------------------------------------------------------------+
|``--data``            |Display correlation data. See *MSLister* documentation.                           |
+----------------------+----------------------------------------------------------------------------------+
|``--verbose``         |Verbose output.                                                                   |
+----------------------+----------------------------------------------------------------------------------+

Additional parameters used with the ``--data`` option (*MSLister* parameters)
`````````````````````````````````````````````````````````````````````````````

See *MSLister* documentation.

Note: There appears to be an error with ``--uvrange``.

+----------------------+------------+-------------------------+-------------------------------------------+
|**Parameter**         |**Default** |**Example**              |**Description**                            |
+======================+============+=========================+===========================================+
|``--datacolumn=str``  |data        |corrected                |Choose the data column to display.         |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--field=str``       |            |                         |Restrict fields.                           |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--spw=str``         |All         |'0:5~10'                 |Restrict spectral windows.                 |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--antenna=str``     |All         |'3&&4;1'                 |Restrict antennas.                         |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--timerange=str``   |All         |'<2014/09/14/10:48:50.0' |Restrict the time range.                   |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--correlation=str`` |All         |XX                       |Restrict polarisations.                    |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--scan=str``        |All         |8                        |Restrict scans.                            |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--uvrange=str``     |All         |'0.1~1klambda'           |Restrict uv range. Seems to compare against|
|                      |            |                         |*u* only. Use with care!                   |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--pagerows=int``    |50          |                         |Set the number of lines to print per page. |
+----------------------+------------+-------------------------+-------------------------------------------+
|``--listfile=str``    |stdout      |                         |Set an output file.                        |
+----------------------+------------+-------------------------+-------------------------------------------+

Example
-------

**Example 1:**

.. code-block:: bash

   $ mslist --verbose --full 1641/2015-04-14_071714_ch00000-00511.ms 

**Example 2:**

.. code-block:: bash

   $ mslist --data --antenna='3&&4' --spw='0:5~10' --correlation='XX,YY' 1641/2015-04-14_071714_ch00000-00511.ms 

