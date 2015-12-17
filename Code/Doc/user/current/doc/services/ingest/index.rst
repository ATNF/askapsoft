Ingest pipeline 
================

The *ingest pipeline* is the central component of the data acquision part
of the system. It is responsible for producing self-contained measurement sets
with the visibility data and appropriate metadata. Under normal circumstances,
the *ingest pipeline* is started by the *Central Processor Manager*. The latter
also has the ability to abort *ingest pipeline*. However, under typical operation
it does not "stop" the ingest pipeline, it stopes when the metadata flowing from
the Telescope Operating System's Executive component indicates an observation has
concluded. The *Executive* assembles configuration file for the *ingest pipeline*
based on the current observation and the *Facility Configuration Manager* 
hostiting the system configuration information which is not expected to change 
frequently from one observation to another.

Execution
---------

The *ingest pipeline* is a C++ application which can both with MPI and as a 
standalone program. Assuming the library path environment variable contains the 
required dependencies it can be started like so::

   <MPI wrapper>  cpingest -c cpingest.in -l cpingest.log_cfg 


Command Line Parameters
~~~~~~~~~~~~~~~~~~~~~~~

The CP manager accepts the following command line parameters:

+-------------------+----------------+-------------+----------------------------------------------------------------+
|**Long Form**      |**Short Form**  |**Required** |**Description**                                                 |
+===================+================+=============+================================================================+
| --standalone      | -s             | No          |Run in standalone/single-process mode (no MPI). In this mode,   |
|                   |                |             |no MPI initialisation or finalisation is done. Therefore, one   |
|                   |                |             |must not use any features which require MPI.                    |
+-------------------+----------------+-------------+----------------------------------------------------------------+
| --config          | -c             | Yes         |After this parameter the file containing the program            |
|                   |                |             |configuration must be provided.                                 |
+-------------------+----------------+-------------+----------------------------------------------------------------+
| --log-config      | -l             | No          |After this optional parameter a Log4cxx configuration file is   |
|                   |                |             |specified. If this option is not set, a default logger          |
|                   |                |             |is used.                                                        |
+-------------------+----------------+-------------+----------------------------------------------------------------+

General overview
----------------

The *ingest pipeline* can be viewed as a collection of tasks executed on the data sequentially. The processing
always starts at the **Source** task, which is responsible for receiving data and for apppropriate formatting and 
tagging. The presence of a source task in the task chain is the requirement. However, the rest of the task chain
can in principle be empty (although in this case the ingest pipeline will not store or process the data, it will
only ingest it). The data writing task is called **MS Sink**. There is no requirement that the sink task should be 
the last in the chain. In fact, any task (except the source task) can be executed more than once with the same or
different parameters. For example, one could write a full resolution data to one measurement set, then average to 
a lower resolution and write the result to a different measurement set. We plan to use this feature eventually
for the on-the-fly calibration. 

.. image:: figures/ingest_overview.png

If used in the parallel (MPI) mode, the same task chain is replicated for each MPI rank to enable parallel processing.
Source tasks are rank aware and increment accordingly the UDP port number which they listen to get the visiblity data. 
This allows us to have separate data streams processed by different ranks of ingest pipeline. By default, each rank does
exactly the same operations according to the task chain but acts on a different portion of data. 
Therefore, the number of measurement
sets written by the *ingest pipeline* is tied down to the number of ranks running the  **MS Sink** task. The file 
name of the created measurement set can have a suffix added to distinguish between measurement sets written by different 
ranks. More than one file is usually required to increase the writing throughput. At the moment, all ranks always receive
data (i.e. execute an appropriate **Source** task). However, later down the processing chain, each
rank can be activated and deactivated by special type tasks which rearrange the data flow and parallelisation.
If the given rank is not active, data are not passed through the task chain for this rank. This can be used to inhibit data
writing for a subset of ranks. In particular, we use this feature of the task interface (see *Merge* task for details)
to aggregade more bandwidth than is defined by the natural hardware-based distribution (one stream normally 
covers 4 MHz of bandwidth produced by a single correlator card). In the future, we envisage also to have 
tasks which would fork data streams to allow more parallel processing (e.g. a separate measurement set for 
calibration can be prepared in parallel). 

Configuration Parameters
------------------------

The program requires a configuration file be provided on the command line. This
section describes the valid parameters. In addition to mandatory parameters which are
always required, individual tasks often have specific parameters which need to be
defined only if a particular task is used.

General parameters
~~~~~~~~~~~~~~~~~~

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|array.name                  |string             |None        |Name of the telescope. It is only used to populate the        |
|                            |                   |            |appropriate field in the measurement set. But must be defined |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|sbid                        |uint               |0           |Scheduling block number. This parameter is unused at the      |
|                            |                   |            |moment.                                                       |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|tasks.tasklist              |vector<string>     |None        |List of tasks to be executed in the same order as they are    |
|                            |                   |            |given here. The first task listed is required to be a source  |
|                            |                   |            |task (e.g. MergedSource). Task names in this list can be      |
|                            |                   |            |chosen freely. The exact functionality is defined by the      |
|                            |                   |            |**type** keyword (see below). This is done to be able to      |
|                            |                   |            |reuse same tasks with different parameters throughout the     |
|                            |                   |            |task chain. We will refer to tasks by their type in this      |
|                            |                   |            |documentation, as they can be named arbitrarily in this list  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|tasks.\ **name**\ .type     |string             |None        |Type of the task with the given **name**\ . The same operation|
|                            |                   |            |can be performed more than once in the processing chain.      |
|                            |                   |            |For example, the same data can be written into multiple files |
|                            |                   |            |with different spectral resolutions. To achieve this, the list|
|                            |                   |            |of tasks contains names which are only references and can be  |
|                            |                   |            |named arbitrarily by the user. Actual physical operations are |
|                            |                   |            |defined by the *type* keyword which is required for each      |
|                            |                   |            |**name** present in the task list. Each **name** can have     |
|                            |                   |            |separate parameters defined (see below). If each task is only |
|                            |                   |            |used once, there is no reason why **name** and **type** could |
|                            |                   |            |not be the same.                                              |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|tasks.\ **name**\ .params.  |varies             |varies      |This is the prefix to define task-specific parameters. Each   |
|                            |                   |            |task **name** listed in the **tasklist** parameter can have   |
|                            |                   |            |a separate set of paramters defined, even if there is more    |
|                            |                   |            |than one task of the same physical **type**\ .                |  
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Beam arrangement
~~~~~~~~~~~~~~~~

Parameters describing the beam arrangement are similar to the *feeds* configuration of :doc:`../../calim/csimulator`.
It is mainly used to initialise **FEED** table of the measurement set, but also used by calculation of the phase centres and
projected baseline coordinates (uvw's) if appropriate tasks are included in the chain. All beams are dual polarisation and
linearly polarised (hard coded). Note, the term *feed* in the context of measurement sets really means *beam*.

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|feeds.n_feeds               |uint               |None        |Number of beams defined in the configuration. Note, only beams|
|                            |                   |            |which are actually written to the measurement set need to be  |
|                            |                   |            |defined.                                                      |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|feeds.feed\ **N**           |vector<double>     |None        |Dimensionless offset of the given beam from the boresight     |
|                            |                   |            |direction (given as [x,y]). Values are multiplied by          |
|                            |                   |            |*feeds.spacing* before being used. This also defined the      |
|                            |                   |            |units (assumed the same for all beams) to get a correct       |
|                            |                   |            |angular quantity.If *feeds.spacing* is not defined, the values|
|                            |                   |            |in this parameter are treated as angular offsets in radians.  |
|                            |                   |            |The offsets should be defined for every **N** from 0 to       |
|                            |                   |            |**feeds.n_feeds - 1**                                         |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|feeds.spacing               |quantity string    |None        |Optional parameter. If present, it determines the dimension   |
|                            |                   |            |and scaling of the beam layout (see above). If not defined,   |
|                            |                   |            |all beam offsets are assumed to be in radians.                |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


Antenna layout
~~~~~~~~~~~~~~


Parameters describing antenna array configuration are similar to *antennas* section 
of :doc:`../../calim/csimulator` configuration.
It is used as a source of data to initialise **ANTENNA** table of the measurement set, but also used by calculation of 
the projected baseline coordinates (uvw's) if appropriate tasks are included in the chain. Only antennas referred to
from the *baselinemap* end up listed in the **ANTENNA** table (and therefore get an index in the measurement set), other
antennas are simply ignored (as they don't participate in the particular measurement and don't contribute to the data 
written or processed past the source task). This section of the configuration is a slice of the antenna information
stored by Facility Configuration Manager (FCM) and often contains parameters which are ignored by the ingest pipeline
(e.g. the aboriginal name or pointing parameters) in addition to antennas unused in the particular experiment.

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|antennas                    |vector<string>     |None        |List of antennas for which this section defines information.  |
|                            |                   |            |Names given here are just logical references used only in the |
|                            |                   |            |names of appropriate configuration parameters. See baselinemap|
|                            |                   |            |for the list of the actually used antennas.                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|antenna.ant.diameter        |quantity string    |None        |Default diameter of antennas, used unless a specific value    |
|                            |                   |            |is defined explicitly for a given antenna.                    |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|antenna.ant.mount           |string             |None        |Default mount of antennas, used unless the mount parameter is |
|                            |                   |            |defined for a given antenna. Supported values are 'equatorial'|
|                            |                   |            |and 'altaz'. We use 'equatorial' for ASKAP to avoid confusion |
|                            |                   |            |of general purpose packages like *casa* which can be used in  |
|                            |                   |            |the short to medium term and for debugging.                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
| the following parameters all have antenna.\ **name** prefix where **name** is an item in of the **antennas** list. Note,   |
| each element of this list should have all compulsory parameters defined.                                                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.name               |string             |None        |Name of the given antenna to be written into **ANTENNA**      |
|                            |                   |            |subtable, use this name in **baselinemap.antennaidx** to tie  |
|                            |                   |            |physical antenna with logical index used by the hardware.     |
|                            |                   |            |The names given in the **antennas** keyword are only used to  |
|                            |                   |            |form the prefix.                                              |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.location.itrf      |vector<double>     |None        |Vector with antenna coordinates in the ITRF frame in metres,  |
|                            |                   |            |i.e. X, Y, Z geocentric coordinates.                          |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.diameter           |quantity string    |see above   |Optional parameter for diameter of the particular antenna. If |
|                            |                   |            |not defined, the default value defined by the                 |
|                            |                   |            |**antenna.ant.diameter** parameter (see above) will be used.  |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.mount              |string             |see above   |Optional mount type for the particular antenna. If not        |
|                            |                   |            |defined, the default value defined by the                     |
|                            |                   |            |**antenna.ant.mount** parameter (see above) will be used.     |
+----------------------------+-------------------+------------+--------------------------------------------------------------+

Correlator modes
~~~~~~~~~~~~~~~~

This section describes the data expected from the correlator. It is largely inherited from BETA and some future changes
are expected in this area to support different frequency tunings of ASKAP. For the parallel environment, the description 
applies to single card only. Different configurations of the input data could change in run time, but all possible
configurations should be defined up front (so the appropriate **SPECTRAL_WINDOW** table can be created).

+----------------------------+-------------------+------------+--------------------------------------------------------------+
|**Parameter**               |**Type**           |**Default** |**Description**                                               |
|                            |                   |            |                                                              |
+============================+===================+============+==============================================================+
|correlator.modes            |vector<string>     |None        |List of supported modes. An exception will be raised if       |
|                            |                   |            |received metadata request a correlator mode which has not     |
|                            |                   |            |been defined in the configuration file. Each mode listed here |
|                            |                   |            |should have the following parameters defined. Modes not listed|
|                            |                   |            |are ignored, even if their parameters are defined.            |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
| All following parameters have correlator.mode.\ **name**\  prefix, where **name** is a mode listed in **correlator.modes** |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.chan_width         |quantity string    |None        |Separation of the channels in frequency, which is always      |
|                            |                   |            |assumed to be equal to the channel width. Full quantity string|
|                            |                   |            |with sign (for inverted spectra) and units.                   |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.interval           |uint               |None        |Correlator cycle time in microseconds.                        |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.n_chan             |uint               |None        |Number of spectral channels handled by a single source task.  |
|                            |                   |            |In parallel environment, this is the number of channels       |
|                            |                   |            |in the single data stream (normally - single card).           |
+----------------------------+-------------------+------------+--------------------------------------------------------------+
|<prefix>.stokes             |vector<string>     |None        |List of products in the polarisation vector in the order as   |
|                            |                   |            |they are to be stored in the measurement set. Although, in    |
|                            |                   |            |principle, all polarisation frames, including incomplete and  |
|                            |                   |            |mixed frames, are supported here and in the definition of     |
|                            |                   |            |correlation products, other frames than full linear are       |
|                            |                   |            |likely to cause problems elsewhere.                           |
+----------------------------+-------------------+------------+--------------------------------------------------------------+


The text below is still to be done (currently a copy of another page)

+----------------------------+----------+------------+--------------------------------------------------------------+
| ice.servicename            | string   | *None*     |The service name (i.e. Ice object identity) for the CP manager|
|                            |          |            |service interface.                                            |
+----------------------------+----------+------------+--------------------------------------------------------------+
| ice.adaptername            | string   | *None*     |The object adapter identity                                   |
+----------------------------+----------+------------+--------------------------------------------------------------+
| monitoring.enabled         | boolean  | false      |Controls the availability of the Ice monitoring provider      |
|                            |          |            |interface. If enabled an Ice interface permits tools such as  |
|                            |          |            |user interfaces and the monitoring archiver to collect        |
|                            |          |            |monitoring data from the service.                             |
+----------------------------+----------+------------+--------------------------------------------------------------+
| monitoring.ice.servicename | string   | *None*     |If monitoring is enabled, this parameter must be specified.   |
|                            |          |            |This parameter provides the name of the monitoring service    |
|                            |          |            |interface that will be registered in the Ice locator service. |
|                            |          |            |An example would be "CentralProcessoMonitoringService".       |
+----------------------------+----------+------------+--------------------------------------------------------------+
| monitoring.ice.adaptername | string   | *None*     |If monitoring is enabled, this parameter must be specified.   |
|                            |          |            |This parameter provides the name of the adapter on which the  |
|                            |          |            |monitoring service proxy object will be hosted. This adapeter |
|                            |          |            |must be configured in the Ice properties section (see example |
|                            |          |            |below).                                                       |
+----------------------------+----------+------------+--------------------------------------------------------------+
| fcm.ice.identity           | string   | *None*     |The Ice object identity of the Facility Configuration Manager |
|                            |          |            |(FCM). This should be qualified with an adapter name if the   |
|                            |          |            |FCM object is not registered as a "well known object".        |
+----------------------------+----------+------------+--------------------------------------------------------------+
| dataservice.ice.identity   | string   | *None*     |The Ice object identity of the Telescope Operating System     |
|                            |          |            |(TOS) Dataservice. This should be qualified with an adapter   |
|                            |          |            |name if the TOS Dataservice is not registeed as w "well known |
|                            |          |            |object."                                                      |
+----------------------------+----------+------------+--------------------------------------------------------------+
| ingest.workdir             | string   | *None*     |The working directory for the ingest pipeline instance. Within|
|                            |          |            |this directory a sub-directory will be created (one for each  |
|                            |          |            |scheduling block executed) for any output files such as       |
|                            |          |            |observation logs and datasets.                                |
+----------------------------+----------+------------+--------------------------------------------------------------+
| ingest.command             | string   | *None*     |The command required to execute the ingest pipeline.          |
+----------------------------+----------+------------+--------------------------------------------------------------+
| ingest.args                | string   | *None*     |The command line arguments to be passed to the ingest         |
|                            |          |            |pipeline.                                                     |
+----------------------------+----------+------------+--------------------------------------------------------------+

Below are the required ICE parameters:

+---------------------------------------+---------+-----------+-----------------------------------------------------+
|**Parameter**                          |**Type** |**Default**|**Description**                                      |
+=======================================+=========+===========+=====================================================+
|ice_properties.Ice.Default.Locator     | string  | *None*    |Identifies the Ice Locator. This will be of the form:|
|                                       |         |           |*IceGrid/Locator:tcp -h <hostname> -p 4061*          |
+---------------------------------------+---------+-----------+-----------------------------------------------------+
|ice_properties.CentralProcessorAdapter\| string  | *None*    |Configures the adapter endpoint that will host the   |
|.Endpoints                             |         |           |actual manager service. Typically this will be: *tcp*|
+---------------------------------------+---------+-----------+-----------------------------------------------------+
|ice_properties.CentralProcessorAdapter\| string  | *None*    |This is the name of the adapter as it is registered  |
|.AdapterId                             |         |           |in the Ice locator service. This will typically be:  |
|                                       |         |           |*CentralProcessorAdapter*                            |
+---------------------------------------+---------+-----------+-----------------------------------------------------+
|ice_properties.CentralProcessorMonitor\| string  | *None*    |Configures the adapter endpoint that will host the   |
|ingAdapter.Endpoints                   |         |           |monitoring provider interface. Typically this will   |
|                                       |         |           |be: *tcp*                                            |
+---------------------------------------+---------+-----------+-----------------------------------------------------+
|ice_properties.CentralProcessorMonitor\| string  | *None*    |This is the name of the adapter (for the monitoring  |
|ingAdapter.AdapterId                   |         |           |provider interface) as it will be registered in the  |
|                                       |         |           |Ice locator service. This will typically be:         |
|                                       |         |           |*CentralProcessorMonitoringAdapter*                  |
+---------------------------------------+---------+-----------+-----------------------------------------------------+

Example
~~~~~~~

.. code-block:: bash

    ########################## Ice Properties ##############################

    # Registry location
    ice_properties.Ice.Default.Locator                  = IceGrid/Locator:tcp -h aktos01 -p 4061

    # Primary object adapter
    ice_properties.CentralProcessorAdapter.Endpoints    = tcp
    ice_properties.CentralProcessorAdapter.AdapterId    = CentralProcessorAdapter

    # Monitoring object adapter
    ice_properties.CentralProcessorMonitorAdapter.Endpoints    = tcp
    ice_properties.CentralProcessorMonitorAdapter.AdapterId    = CentralProcessorMonitorAdapter

    # Other misc parameters
    ice_properties.Ice.MessageSizeMax                   = 131072
    ice_properties.Ice.ThreadPool.Server.Size           = 4
    ice_properties.Ice.ThreadPool.Server.SizeMax        = 16

    ################## CP Manager Specific Properties ######################

    # Object identity and proxy to use for the CP manager ICE object
    ice.servicename                 = CentralProcessorService
    ice.adaptername                 = CentralProcessorAdapter

    # Monitoring provider configuration
    monitoring.enabled              = true
    monitoring.ice.servicename      = CentralProcessorMonitorService
    monitoring.ice.adaptername      = CentralProcessorMonitorAdapter

    # FCM config
    fcm.ice.identity                = FCMService@FCMAdapter

    # Scheduling block service
    dataservice.ice.identity        = SchedulingBlockService@DataServiceAdapter

    # Ingest working directory
    ingest.workdir                  = /scratch2/datasets

    # Ingest pipeline command and arguments
    ingest.command                  = /askap/cp/cpingest.sh
    ingest.args                     = -s -c cpingest.in -l /askap/cp/config/cpingest.log_cfg
