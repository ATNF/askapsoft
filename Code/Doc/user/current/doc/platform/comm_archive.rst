Commissioning Archive Platform
==============================

Recent datasets
---------------

Scheduling block measurements sets that have been recently observed,
or are the subject of current processing, are kept on the Galaxy
Lustre file system at
*/astro/askaprt/askapops/askap-scheduling-blocks*. Each scheduling
block directory contains a measurement set, the ingest pipeline parset
and log, as well as other files indicating the state of the
archiving.

Archiving of measurement sets
-----------------------------

After the observation is complete, new datasets are archived on the
commisisoning archive. This is tape and disk-based storage aimed at
keeping raw and some processed data during ASKAP commissioning.

The observations are archived by tar-ing the entire scheduling block
directory. After this is copied to the archive, the measurement set
within the directory on /astro is removed, and a file called
ARCHIVED is created.

There is a RESTORE.SH script that ASKAP operations will make use of to
restore the measurement set from the archive. General users cannot run
this, as the askap-scheduling-blocks directory is not writeable.

If a scheduling block is being restored, and you want to run the
pipeline processing (:doc:`../pipelines/index`) as soon as it is
available, you can use the **stage-processing.sh** script in the
*askaputils* module (see :doc:`modules`). The calling syntax is::

  stage-processing.sh myconfig.sh <jobID>

where <jobID> is the slurm job ID of the restore job. Run
"stage-processing.sh -h" for more information.

All BETA datasets were archived in the same fashion, and are available
in their own directory on both the commissioning archive
(in beta-scheduling-blocks rather than askap-scheduling-blocks).

Viewing and accessing the archive
---------------------------------

The commissioning archive is visible through the Pawsey Data portal
https://data.pawsey.org.au. You can log in with your Pawsey
credentials, and navigate through the directory structure. It is
possible to download via the web client as well

There are also command-line tools available on Galaxy to access the
archive and retrieve data. While the preferred way is to have the data
restored to the standard location via the RESTORE.SH script, there are
situations where downloading the datasets yourself is
desirable. *(Remember to moderate your usage of /scratch2!)*

pshell
......

The **pshell** command is the preferred way to access the
archive. This is made available via the ``askaputils`` module, which
can be loaded via::

  module load askaputils

(adding this to your ~/.bashrc file allows it to be available
automatically).

It provides a shell-like environment to navigate around the remote and
local filesystems, and commands to get or put data. Useful features
include tab-completion and a progress indicator.

When starting pshell, you will be presented with the following
prompt::

   === pshell: type 'help' for a list of commands ===
   pawsey:offline>

You can login with your Pawsey credentials, and obtain data in the
following way::

  pawsey:offline>login
  pawsey:/projects>get ASKAP Commissioning Data/askap-scheduling-blocks/3046

The available commands can be seen by typing "help" or "help
<command>" for more details on a particular command::

  pawsey:/projects>help
  
  Documented commands (type help <topic>):
  ========================================
  cd        exit  help  login   ls         put   rm
  debug     file  lcd   logout  mkdir      pwd   rmdir
  delegate  get   lls   lpwd    processes  quit  whoami

There are different commands for moving in the local and remote
filesystem. The remote filesystem uses the familar 'ls', 'cd' etc
commands, while the local filesystem has an additional 'l' at the
start ('lls', 'lcd' etc).

Finally, a useful command is 'delegate', which stores your login
details for some period of time. This is useful for scripting
pshell. To use, give delegate a number of days over which to store
your credentials, and it will tell you when it will expire::

  pawsey:/projects>delegate 30
  Delegating until: 25-Mar-2017 07:55:35


ashell
......

The original tool used to connect to the archive was ashell.py. It has
a similar (although not identical) interface to pshell, without
tab-completion and a different scripting interface. We have kept
ashell available for backwards-compatibility reasons.

This can be loaded manually using::
	
	module load ashell
	
You can add this to your ~/.bashrc file for automatic loading; see the section "Setting up your account"
in the :doc:`processing` documentation page. Once loaded you can start the interface with::

	ashell.py
	
After starting ashell you should be presented with the the following prompt::

	pawsey:offline>
	
To download the BETA scheduling block 50 data to your current folder use the following commands::

	pawsey:offline>login
	pawsey:online>get /projects/ASKAP Commissioning Data/beta-scheduling-blocks/50.tar

You will then have a local copy of the file 50.tar, and you will need
to un-tar it before you can use the measurement set therein.
        
Ashell also uses the concept of remote and local folders, although
with a different syntax to pshell. the remote folder is set by the 'cf <path>' (Change Folder)
command, you can check the current remote folder with the 'pwf' (Print Working Folder)
command. The local folder is set by the 'cd <path>' command and can be
checked with 'pwd'.

Delegation is also available in the same manner as pshell. Quick help
is also available via 'help' and 'help <command>'. 

Additional Information
----------------------

* `PawseyData Help <https://support.pawsey.org.au/documentation/display/US/Data+Documentation>`_
* `Pawsey Data Services Command Line Client <https://support.pawsey.org.au/documentation/display/US/Use+the+Command+Line>`_
