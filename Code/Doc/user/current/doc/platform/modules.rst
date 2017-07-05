Environment Modules
===================

The `Environment Modules`_ software is used to manage the user's environment, dynamically
modifying the user's environment to allow the execution of various software packages.
Both the Cray software and the ASKAP specific software uses this approach. Typically,
modules will alter the user's *PATH* and/or *LD_LIBRARY_PATH*, and other environment variables
that may be necessary to execute some software package.

.. _Environment Modules: http://modules.sourceforge.net/

You can browse all software packages that can be *loaded* using this mechanism with the
following command::

    module avail

In order for the ASKAP specific modules to appear in this list you must have already
executed the following command. However this command should have been added to your
*.bashrc* file per the instructions in the section :doc:`processing`::

    module use /group/askap/modulefiles

The ASKAP modules appear in their own section in the output from the *module avail*
command::

  ------------------------------------------------------------------ /group/askap/modulefiles -----------------------------------------
  aces/0.3(default)                askapsoft/0.4.0                  askapsoft/0.7.2                  bbcp/13.05.03.00.0(default)
  ashell                           askapsoft/0.4.1                  askapsoft/0.7.3                  casa/30.1.11097-001-64b
  askapdata/current(default)       askapsoft/0.5.0                  askapsoft/0.8.0                  casa/4.3.0-el5
  askappipeline/r7046              askapsoft/0.6.1                  askapsoft/0.8.1                  casa/41.0.24668-001(default)
  askappipeline/r7054              askapsoft/0.6.2                  askapsoft/0.9.0                  casa/42.2.30986-1-64b
  askappipeline/r7064              askapsoft/0.6.3                  askapsoft/0.9.1(default)         deprecated-python/2.7.6(default)
  askappipeline/r7067(default)     askapsoft/0.7.0                  askapsoft/r6822                  tmux/1.8(default)

These modules are:

* **askapsoft** - The ASKAPsoft Central Processor applications binaries and libraries
* **askapdata** - Measures data for ASKAPsoft
* **askappipeline** - Pipeline scripts for processing ASKAP/BETA observations
* **askaputils** - General utility scripts (*pshell* for data
  transfer - see :doc:`comm_archive`, plus other utility scripts)
* **casa** - NRAO's CASA software package (see :doc:`casa`)
* **bbcp** - BBCP Fast file copy (see :doc:`externaltransfer`)
* **ashell** - Connection to the commissioning archive (see :doc:`comm_archive`)
* **aces** - Python libraries used for the ACES tools (see `ACES wiki page`_) and the **acesops** module.
* **acesops** - A tagged version of the ACES subversion repository, designed to give a static set of ACES tools to enable reproducibility of processing.

  .. _ACES wiki page: https://confluence.csiro.au/display/ACES/Getting+started+with+ACES+tools+on+Galaxy

You can use the *module whatis* command to obtain a description of the module::

    $ module whatis askapdata
    askapdata            : Measures data for ASKAPsoft

You can use the *module list* command to view already loaded modules, and you will note
many Cray modules already loaded. A given module can be loaded like so::

    module load askapsoft

And can be unloaded just as simply::

    module unload askapsoft

Notice the *askapsoft* module has multiple versions, with the version
number indicated, for instance *askapsoft/0.8.0*.  You will also see
one is tagged *"(default)"*". If you execute the *"module load
askapsoft"* command you will load the default version. You can load a
specific version by specifying the version in the *module load*
command::

    module load askapsoft/0.9.0

Often it is useful to understand exactly how a module will modify your environment. The
*module display* command can be used to determine this. For instance::

    $ module display askapsoft
    -------------------------------------------------------------------
    /group/askap/modulefiles/askapsoft/0.9.1:

    module-whatis    ASKAPsoft software package 
    prepend-path     PATH /group/askap/askapsoft/0.9.1/bin 
    prepend-path     LD_LIBRARY_PATH /group/askap/askapsoft/0.9.1/lib64 
    -------------------------------------------------------------------

The above shows that the askapsoft module will prepend the ASKAPsoft *bin* directory to
your *PATH* environment variable, and the *lib64* directory to your *LD_LIBRARY_PATH*.
Likewise the askapdata module can be inspected::

    $ module display askapdata
    -------------------------------------------------------------------
    /group/askap/modulefiles/askapdata/current:

    module-whatis   Measures data for ASKAPsoft 
    setenv          AIPSPATH /group/askap/askapdata/current
    -------------------------------------------------------------------

This shows that the *AIPSPATH* will be set to point to the directory containing measures
data.
