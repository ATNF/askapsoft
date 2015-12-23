TCPSink     
======

TCPSink task sends data via socket to vispublisher which further distributes a selection of data to user clients 
such as vis and spd. Two-stage data distribution approach allows us to decouple of real time application such as
ingest pipeline from user-defined application capable of creating variable processing load. Due to the shear amount
of data we do not expect that this task can be scaled up to full ASKAP. But nothing stops us to continue using it
with, e.g. frequency-averaged data. Also note that this task can only be used for a single data stream. The
code was specifically designed to be as graceful as possible on the core functionality of the ingest pipeline
if send operaton cannot keep up (i.e. monitoring of the data should be affected first before normal functionality
of the ingest pipeline).

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the table below) where
**name** is an arbitrary name assigned to this task and used in *tasklist* (this allows us
to run the same task more than once with different parameters. The type of
the task defined by tasks.\ **name**\ .type should be set to *TCPSink*.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|dest.hostname               |string             |None        |Hostname to send the data to, i.e. machine which runs         |
|                            |                   |            |vispublisher. This task doesn't fail in the case of any       |
|                            |                   |            |network error.                                                |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|dest.port                   |string             |None        |Port number to use. It is passed as a string to asio library. |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## TCPSink ##############################

    tasks.tasklist = [MergedSource, Merge, CalcUVWTask, FringeRotationTask, MSSink, TCPSink]

    # sink task sending the data for monitoring via vis and spd
    tasks.TCPSink.params.dest.hostname = aktos11.atnf.csiro.au
    tasks.TCPSink.params.dest.port = 9001
    # type of the task
    tasks.TCPSink.type = TCPSink

