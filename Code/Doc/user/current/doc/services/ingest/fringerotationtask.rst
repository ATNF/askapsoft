FringeRotationTask
==================

FringeRotationTask does delay and/or phase tracking under control of ingest pipeline. This is an
interim solution used for BETA and early ADE with ingest pipeline running at MRO. We expect to do
fringe rotation outside of SDP (i.e. by the Telescope Operating System configuring the hardware 
fringe rotator) following the transition of ingest pipeline to the Pawsey centre (i.e. this tight
real time control loop is not expected to extend across the MRO-Pawsey boundary). The task itself
is generic for various ways of doing fringe rotation. It implements a simple delay model (using
casacore's measures and simple geometry/frame conversions) and computes delays and rates for each
antenna present in the **ANTENNA** table with respect to the Earth's centre. These delay and 
rates are passed to chosen  *fringe rotation method* class (see **method** keyword) which do the 
low level work (different methods do tracking via software, via hardware or both with various
degrees of accuracy). The parameters required depend on the method used. All currently available
methods apply delay and rates with respect to a chosen reference antenna to minimise the adjustments needed.
The task relies on the correct phase centre being set (either via metadata or parset, depending 
on the source task used), accurate timing of the data and, in most cases, correct frequency axis.
Note, this task currently supports a single data stream only. Therefore, it should always be after
merge task in the processing chain.


Configuration Parameters
------------------------

Ingest pipeline requires a configuration file to be provided on the command line. This
section describes the valid parameters applicable to this particular task.
These parameters need to be defined only if this task is used. As for all tasks, parameters are taken
from keys with tasks.\ **name**\ .params prefix (not shown in the tables described below) where
**name** is an arbitrary name assigned to this task and used in *tasklist*\ .
The type of the task defined by tasks.\ **name**\ .type should be set to *FringeRotationTask*.

General Parameters
~~~~~~~~~~~~~~~~~~

The table below contains general parameters which are required for all *fringe rotation methods*\ . Parameters specific
to some methods are given in the following subsections. The **swdelays** method is fully software-based and does not require
any additional parameters to those listed in this table.

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|method                      |string             |None        |The choice of the *fringe rotation method* to physically      |
|                            |                   |            |apply the delay model. See subsections below for the list of  |
|                            |                   |            |available methods.                                            |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|fixeddelays                 |vector<double>     |empty vector|This is the obsolete way of specifying fixed delays. This     |
|                            |                   |            |parameter is understood, but not used. A warning is given in  |
|                            |                   |            |the log if this parameter is present (but value is simply     |
|                            |                   |            |ignored).                                                     |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|calcuvw                     |bool               |true        |If true (default), this task also calculates UVWs rendering   |
|                            |                   |            |CalcUVWTask unnecessary. The math required to calculate UVWs  |
|                            |                   |            |and delay model is very similar (just the reference frame is  |
|                            |                   |            |different). Therefore, doing these calculations at the same   |
|                            |                   |            |place allows us to improve the performance.                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|refant                      |string             |None        |Reference antenna used in delay and rate application. This    |
|                            |                   |            |parameter is actually defined at the *method* level, but all  |
|                            |                   |            |currently available methods work with differential delay and  |
|                            |                   |            |rates (w.r.t. a chosen antenna), so this keyword is described |
|                            |                   |            |in the general section. The antenna should be one of names    |
|                            |                   |            |defined in the array layout. Note, for the **swdelays** method|
|                            |                   |            |(which does not talk to the hardware at all) the reference    |
|                            |                   |            |antenna is not required to be physically present, but must be |
|                            |                   |            |defined in the antenna table. For other methods, the ingest   |
|                            |                   |            |receiver script should be able to talk to the appropriate     |
|                            |                   |            |hardware.                                                     |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Available methods
~~~~~~~~~~~~~~~~~

The table below lists available methods. Note, some methods are BETA-specific and some are ADE-specific. The difference arises
because of two factors. First, only BETA DRx has a functionality to apply delays through the offset pointer in the memory
buffer. And second, there are substantial differences in the frequency conversion (ADE system doesn't have LOs) affecting
phase tracking. Note also, that any changes to the frequency handling (for example to benefit from flexibility supported by
ADE hardware which is currently not covered by software) would require changes in the fringe rotation methods.

+----------------------------+-------------+------------+--------+----------+---------------------------------------------------+
|**Method**                  |**Needs inge\|**Tracks ph\|**Tracks|**Tracks \|**Description**                                    |
|                            |st receiver**|ases**      |delays**|rates**   |                                                   |
+============================+=============+============+========+==========+===================================================+
|drxdelays                   |Yes          |Yes         |Yes     |No        |BETA-specific method to correct for delays using   |
|                            |             |            |        |          |DRx 1.3ns steps. Corresponsing phases are corrected|
|                            |             |            |        |          |in software, phase rates are ignored. Optionally,  |
|                            |             |            |        |          |the residual delay (e.g. <1.3 ns) can be tracked   |
|                            |             |            |        |          |in software.                                       |
+----------------------------+-------------+------------+--------+----------+---------------------------------------------------+
|hwanddrx                    |Yes          |Yes         |Yes     |Yes       |BETA-specific method to correct for delays using   |
|                            |             |            |        |          |1.3ns DRx steps and for phase rates using the      |
|                            |             |            |        |          |hardware fringe rotator. Phases and residual       |
|                            |             |            |        |          |delays (e.g. <1.3ns) are corrected in software.    |
+----------------------------+-------------+------------+--------+----------+---------------------------------------------------+
|swdelays                    |No           |Yes         |Yes     |No        |Software-based phase and delay tracking. Phase     |
|                            |             |            |        |          |rates are ignored. Tested with ADE only, but should|
|                            |             |            |        |          |work with small BETA baselines too as it works with|
|                            |             |            |        |          |sky freqiuencies and does not touch the hardware ( |
|                            |             |            |        |          |and, therefore, differences in frequency conversion|
|                            |             |            |        |          |should not matter).                                |
+----------------------------+-------------+------------+--------+----------+---------------------------------------------------+
|hwade                       |Yes          |Yes         |Yes     |Yes       |ADE-specific method to correct for delays and phase|
|                            |             |            |        |          |rates using hardware fringe rotator. Phases and    |
|                            |             |            |        |          |residual delays (0.2ns is the smallest possible    |
|                            |             |            |        |          |delay step of the current hardware) are corrected  |
|                            |             |            |        |          |in software.                                       |
+----------------------------+-------------+------------+--------+----------+---------------------------------------------------+

Common parameters for hardware-based methods
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This subsection summarises common parameters for all methods communicating with the ingest receiver, i.e. all but **swdelays**\ .
Methods interacting with the hardware are doing so by sending a message over Ice to the **OSL** script called ingest receiver
(note, the ADE and BETA versions are slightly different). This script decodes the message and runs an appropriate low-level
**OSL** script (also called auto-script) setting the values to either DRx or hardware fringe rotator for a particular antenna.
Antenna names written into **ANTENNA** table of the measurement set (i.e. see the array layout section of the main
:doc:`index` page for the information how to define the names) are used in the message to the ingest receiver and, therefore, in
the **OSL** scripts. Therefore, the names should be that recognised by the Telecope Operating System (TOS). All methods which
require communication with ingest receiver (see the table above), need appropriate *Ice* configuration in the parameters. These
parameters are summarised in the table below. Also, it is the nature of the control loop based on the timestamp supplied by
the correlator that it is always at least a couple of cycles late. In addition, some elements of the hardware (e.g. BETA DRx) or
software (current asynchonous implementation of the auto-scripts for ADE) make it difficult to perfectly synchronise application
of the new hardware settings with the visibility data. Therefore, there is an option to flag a given additional number of correlator
cycles following the update of hardware. Note, the tolerances are likely to be different for the DRx and the hardware fringe rotator,
but only a single waiting period is implemented for simplicity.

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|cycles2skip                 |unsigned int       |5           |Number of additional correlator cycles to flag following the  |
|                            |                   |            |receipt of the reply message from ingest receiver. This param\|
|                            |                   |            |eter is required to account for additional latencies in the   |
|                            |                   |            |system. Set it to zero to avoid any extra flagging (if the    |
|                            |                   |            |data happen to be good).                                      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|ice.locator_host            |string             |None        |Host name for the machine running *Ice* locator service.      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|ice.locator_port            |string             |None        |Port number for the *Ice* locator service.                    |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|icestorm.topicmanager       |string             |None        |Topic manager for communication channel to the ingest receiver|
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|icestorm.outtopic           |string             |None        |Topic name for the ingest pipeline to the OSL script messages,|
|                            |                   |            |i.e. outgoing traffic.                                        |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|icestorm.intopic            |string             |None        |Topic name for the OSL script to the ingest pipeline or       |
|                            |                   |            |reply messages (sent when the request is fulfilled with the   |
|                            |                   |            |actual time of application).                                  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+



Additional parameters for *drxdelays*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The table below describes configuration parameters specific to the **drxdelays** fringe rotation method.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|drxdelaystep                |unsigned int       |0           |Tolerance in DRx delay steps (i.e. in 1.3ns steps) describing |
|                            |                   |            |when the old DRx delay setting can be reused. If desired      |
|                            |                   |            |diverges from the current setting by more than the tolerance  |
|                            |                   |            |DRx update is initiated (may take several correlator cycles). |
|                            |                   |            |Higher value ensures less data are flagged for the price of   |
|                            |                   |            |larger residual delay. The default of zero forces update every|
|                            |                   |            |time the delay correction changes by 1.3 ns.                  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|trackresidual               |boolean            |true        |If true, the task will correct for the residual delay (i.e.   |
|                            |                   |            |up to 1.3ns or more, if the previous parameter is set) in     |
|                            |                   |            |software. The accuracy of software-based correction is limited|
|                            |                   |            |by the spectral resolution.                                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Additional parameters for *hwanddrx*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The table below describes configuration parameters specific to the **hwanddrx** fringe rotation method. Note, in constrast to
**drxdelay** method, residual delays are always corrected in software.


+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|drxdelaystep                |unsigned int       |0           |Tolerance in DRx delay steps (i.e. in 1.3ns steps) describing |
|                            |                   |            |when the old DRx delay setting can be reused. If desired      |
|                            |                   |            |diverges from the current setting by more than the tolerance  |
|                            |                   |            |DRx update is initiated (may take several correlator cycles). |
|                            |                   |            |Higher value ensures less data are flagged for the price of   |
|                            |                   |            |larger residual delay. The default of zero forces update every|
|                            |                   |            |time the delay correction changes by 1.3 ns.                  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|frratestep                  |unsigned int       |20          |Tolerance in phase rate (in the hardware units). When the     |
|                            |                   |            |desired rate diverges more than this value from the current   |
|                            |                   |            |a fringe rotator update is initiated for a particular antenna.|
|                            |                   |            |Descreasing the value will lead to more aggressive flagging ( |
|                            |                   |            |due to more frequent updates), but less decorrelation in the  |
|                            |                   |            |visibility data).                                             |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|drxmidpoint                 |int                |2048        |Middle of the range for DRx delays. Delay tracking via DRx is |
|                            |                   |            |implemented using the memory pointer offset in the buffer.    |
|                            |                   |            |This offset is an unsigned quantity while the delay has the   |
|                            |                   |            |sign. As only the relative delay matters, the signed delay is |
|                            |                   |            |implemented by offsetting the midpoint. An adjustment to      |
|                            |                   |            |midpoint may be handy in the case of equatorial sources (as   |
|                            |                   |            |delay may fall out of range, otherwise).                      | 
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|flagoutofrange              |boolean            |true        |If true, antennas with out of range hardware settings are     |
|                            |                   |            |flagged. Otherwise, the software will apply as large as (or as|
|                            |                   |            |small as) the value which is supported by the hardware.       |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|updatetimeoffset            |int                |None        |A fudge factor subtracted from the timestamp when the hardware|
|                            |                   |            |fringe parameters were updated. The hardware fringe rotation  |
|                            |                   |            |is done in the beamformer which has a different clock domain  |
|                            |                   |            |to the correlator. This parameter allows to adjust update time|
|                            |                   |            |for possible differences. The value can be both positive and  |
|                            |                   |            |negative with the units of microseconds.                      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Additional parameters for *hwade*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The table below describes configuration parameters specific to the **hwade** fringe rotation method. It is very similar to 
**hwanddrx** method but adapted to ADE hardware with the following differences:

 * Delay tracking via hardware fringe rotator (as DRx no longer has this functionality)
 * No out of range flagging as the library works with floating point and should support all settings which could arise in operations

The minimum increments of fringe rotator parameters are exactly the same for ADE and BETA. These hardware units are used in the
interface between ingest receiver and the task (so the code can be reused) and define the meaning of the tolerance parameters
(as for BETA). 

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|delaystep                   |unsigned int       |0           |Tolerance in delay steps, one step is approximately 0.206ns.  |
|                            |                   |            |The fringe rotator setting is updated if desired delay is off |
|                            |                   |            |by more than this tolerance from the actual setting. Zero     |
|                            |                   |            |means update delay every time it changes by a minimal step.   |
|                            |                   |            |The residual delay is always tracked in software. Reducing    |
|                            |                   |            |this tolerance might be little bit more accurate, but more    |
|                            |                   |            |data will be flagged. This is a similar parameter to          |
|                            |                   |            |**drxdelaystep** understood by the previous method, but it has|
|                            |                   |            |different units (as it applies to fringe rotator as opposed to|
|                            |                   |            |digital receiver.                                             |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|frratestep                  |unsigned int       |20          |Tolerance in phase rate (in about 0.0248 deg/s steps). When   |
|                            |                   |            |the desired rate diverges more than this value from the set   |
|                            |                   |            |value the update is initiated for a particular antenna.       |
|                            |                   |            |Descreasing the value will lead to more aggressive flagging ( |
|                            |                   |            |due to more frequent updates), but less decorrelation in the  |
|                            |                   |            |visibility data due to wrong rate applied).                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|updatetimeoffset            |int                |None        |A fudge factor subtracted from the timestamp when the hardware|
|                            |                   |            |fringe parameters were updated. The hardware fringe rotation  |
|                            |                   |            |is done in the beamformer which has a different clock domain  |
|                            |                   |            |to the correlator. This parameter allows to adjust update time|
|                            |                   |            |for possible differences. The value can be both positive and  |
|                            |                   |            |negative with the units of microseconds.                      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|freq_offset                 |quantity string    |0.0Hz       |This is a fudge offset added to central frequency deduced from|
|                            |                   |            |data before it is used to find out the phase component        |
|                            |                   |            |corresponding to the inserted delay as part of delay tracking |
|                            |                   |            |(effective LO). Note, there are additional fudge factors      |
|                            |                   |            |hard coded (e.g. 5 fine channel offset). The main reason why  |
|                            |                   |            |this parameter was introduced is to be able to reconfigure    |
|                            |                   |            |ingest to work with any number of available correlator blocks |
|                            |                   |            |without recompilation of the code. The correctness of the     |
|                            |                   |            |phase tracking depends on which part of the band we are inges\|
|                            |                   |            |ting and, therefore, a single fudge factor in the in the Sour\|
|                            |                   |            |ce task is not sufficient.                                    |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|nthreads                    |unsigned int       |10          |Number of parallel threads used to apply residual phases. This|
|                            |                   |            |task is one of the serial bottlenecks in the early science    |
|                            |                   |            |processing. Due to temporary nature, no effort has been dedic\|
|                            |                   |            |ated towards refactoring of this code. Therefore, a quick and |
|                            |                   |            |dirty way with shared memory parallelism has been used to be  |
|                            |                   |            |able to go beyond 12 antennas. This parameter controls the    |
|                            |                   |            |number of parallel threads used. Note, it is impossible to get|
|                            |                   |            |to pre-Novmeber 2017 fully serial version without recompilati\|
|                            |                   |            |on of the code.                                               |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## FringeRotationTask ##############################

    tasks.tasklist = [MergedSource, Merge, CalcUVWTask, FringeRotationTask, MSSink, TCPSink]

    # immediately unflag the data when the reply is received from ingest receiver
    tasks.FringeRotationTask.params.cycles2skip = 0

    # update delays when they diverge by more than 500 hardware units
    tasks.FringeRotationTask.params.delaystep = 500

    # just to give an example, this is the obsolete way of specifying fixed delays. It is not supported any more.
    # Use delay definition in the antenna section (see the main page of ingest pipeline documentation for details)
    # fixed delays in nanoseconds, in the order of increasing antenna indices
    # values below are fixed delays used for antennas ak02, ak04, ak05, ak12, ak13 and ak14
    # (in that order) in the November commissioning run  
    #tasks.FringeRotationTask.params.fixeddelays = [-198.004385, 0, 275.287053, -1018.02295, -1077.35682, 2759.82581]

    # update rates when they diverge by more than 50 hardware units
    tasks.FringeRotationTask.params.frratestep = 50

    # Ice parameters to communicate with ingest receiver OSL script
    tasks.FringeRotationTask.params.ice.locator_host = aktos10
    tasks.FringeRotationTask.params.ice.locator_port = 4061
    tasks.FringeRotationTask.params.icestorm.intopic = frt2ingest
    tasks.FringeRotationTask.params.icestorm.outtopic = ingest2frt
    tasks.FringeRotationTask.params.icestorm.topicmanager = IceStorm/TopicManager@IceStorm.TopicManager

    # fringe rotation method class (hwade = ADE h/w fringe rotator + tracking residual delays in s/w)
    tasks.FringeRotationTask.params.method = hwade

    # reference antenna, the name should be one of the defined antenna names
    tasks.FringeRotationTask.params.refant = AK04

    # assume that the fringe rotator and correlator are perfectly synchronised
    tasks.FringeRotationTask.params.updatetimeoffset = 0

    # type of the task
    tasks.FringeRotationTask.type = FringeRotationTask

