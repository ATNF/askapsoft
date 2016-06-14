.. _purgepolicy:
.. _Pawsey file systems: https://portal.pawsey.org.au/docs/Supercomputers/File_Systems
.. _Transferring files: https://portal.pawsey.org.au/docs/Supercomputers/Transferring_Files#Data_Transfers
.. _transferring large files: https://portal.pawsey.org.au/docs/Supercomputers/Transferring_Files#Large_Data_Transfers
.. _transferring small files: https://portal.pawsey.org.au/docs/Supercomputers/Transferring_Files#Small_Data_Transfers
.. _Removing files on /scratch and /group: https://portal.pawsey.org.au/docs/Supercomputers/File_Systems_with_munlink

scratch2 Purge Policy
=====================

The ``scratch2`` 1.4PB Lustre file system at the Pawsey Centre is allocated to the ASKAP project (1 PB) and the MWA (0.4 PB). Hard quotas are not in place, as a certain amount of 
flexibility between ASKAP and MWA usage benefits both projects so long as total usage is managed. Manually managing free space on ``scratch2`` is not practicable. Due to the configuration and characteristics 
of the Lustre file system, file writes can fail even if the total free space is greater than the file size required. This is primarily due to individual Lustre Object Storage Targets (OSTs) running out of space. 
To maintain operational stability of the ``scratch2`` filesystem, and ASKAP, free space must be maintained at a reasonable level.

To address this, an automated file purging policy is in place on ``scratch2``. This automatically removes files with an **access time older than a set value**. Analysis shows that a most recent 
access time of 30 to 45 days gives reasonable benefits. It should be noted that the term *scratch* means temporary and not intended for long term data archiving. However, 
alternative locations for your data are available and you are urged to regularly examine your data usage on ``scratch2`` and **selectively** attend to the data you wish to keep.

* `Pawsey file systems`_
* `Transferring files`_

The Policy
----------
The current policy is that all files are subject to purging once they are older than 30 days unless they are within the list of exclusions. 
The list of exclusions include ingested measurement sets, CASDA files, busy week datasets, MWA files, and any other files deemed excluded by Pawsey administrators.

User directories **are** subject to purging.

Alternative Locations for Your Files
------------------------------------

group
`````
The advice from Pawsey admin is that you should be placing ``scratch2`` data that you want to keep in ``/group/astronomy856/$USER``. The ``group`` filesystem has a total quota of 10TB for the astronomy856 group.

**Small amounts of data** can be copied directly::

    you@galaxy2> scp data.tar.gz /group/astronomy856/$USER/data.tar.gz
    
See the Pawsey documentation for `transferring small files`_ for more information.

**Large amounts of data** should be copied via the data transfer nodes with a batch job and ``copyq``. There is no efficiency gain for 
internal transfers, but it is done in the background and more robust::

    you@galaxy2> sbatch --cluster=zeus --partition=copyq script-data-copy.sh

See the Pawsey documentation for `transferring large files`_ for more information.

Commissioning archive
`````````````````````
Ingested measurement sets owned by ``askapops`` residing on ``scratch2`` will be purged according to the Commissioning Archive policy which is independent. Once this data has been archived it will be removed, however, 
measurement sets can be retrieved from the archive by ASKAP Science Operations staff. Requests should be submitted to ASKAP Support, specifying the scheduling block ID (email askap-datasup@csiro.au).

See :doc:`comm_archive` for more information.

CASDA
`````
For the Early Science program, CASDA should be used to store science products and the associated raw measurement sets and calibration results.

Removing Your scratch2 Files
----------------------------
Once you have moved the files you need to keep from ``scratch2`` to another location it would be good to remove them if you or no-one else requires them. This will immediately free up space
without waiting for the purge period to elapse. Using the normal ``rm`` command is not recommended on a Lustre filesystem like ``scratch2`` and the ``munlink`` command should be used instead.

See the Pawsey documentation for `Removing files on /scratch and /group`_ and the ``munlink`` command.

This process also applies to removing files from ``group``.
