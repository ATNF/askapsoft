Commissioning Archive Platform
==============================

.. note:: A cache of measurement sets for recently observed scheduling
          blocks is kept on the Galaxy Lustre file system at the
          following paths:
          */scratch2/askap/askapops/beta-scheduling-blocks* (for BETA
          data) and */scratch2/askap/askapops/askap-scheduling-blocks*
          (for ASKAP/ADE data) - You may wish to check there first as
          access to this location is faster than access to the
          commissioning archive, where the files may reside on tape
          media.

The archive can be accessed from Galaxy via a command line utility called ashell.py. This can
be loaded manually using::
	
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
        
Ashell uses the concept of remote and local folders, the remote folder is the directory
where the files are located in the archive, this is set by the 'cf <path>' (Change Folder)
command, you can check the current remote folder with the 'pwf' (Print Working Folder)
command. The local folder is set by the 'cd <path>' command and can be checked with 'pwd'.

Quick help is also available via 'help' and 'help <command>'

Additional Information
----------------------

* `PawseyData Help <https://support.pawsey.org.au/documentation/display/US/Data+Documentation>`_
* `Pawsey Data Services Command Line Client <https://support.pawsey.org.au/documentation/display/US/Use+the+Command+Line>`_
