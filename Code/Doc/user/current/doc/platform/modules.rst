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

    ------------------------------------------------------------------- /group/askap/modulefiles --------------------------------------------------------------------
    aces/0.5(default)             askapdata/current(default)    askappipeline/0.20.1          askapsoft/0.19.7              casa/4.7.0-el6
    aces/0.5b                     askappipeline/0.19.0          askappipeline/0.20.3(default) askapsoft/0.20.0              casa/5.0.0-218.el6
    aces/0.6                      askappipeline/0.19.2          askappipeline/cle6-dev-9419   askapsoft/0.20.1              casa/5.1.1-5.el6
    acesops/r47210                askappipeline/0.19.3          askapservices/0.15.0(default) askapsoft/0.20.3(default)     casa/5.1.1-5.el7(default)
    acesops/r47349(default)       askappipeline/0.19.4          askapsoft/0.19.0              askapsoft/cle6-dev-9419       karma/1.7.25(default)
    aoflagger/2.10.0(default)     askappipeline/0.19.5          askapsoft/0.19.2              askapsofthook                 tmux/1.8(default)
    ashell                        askappipeline/0.19.6          askapsoft/0.19.4              askaputils
    askap-cray/current            askappipeline/0.19.7          askapsoft/0.19.5              askapvis/current(default)
    askapcli/current(default)     askappipeline/0.20.0          askapsoft/0.19.6              bbcp/13.05.03.00.0(default)    

These modules are:

* **askapsoft** - The ASKAPsoft Central Processor applications binaries and libraries
* **askapdata** - Measures data for ASKAPsoft
* **askappipeline** - Pipeline scripts for processing ASKAP/BETA observations
* **askaputils** - General utility scripts (*pshell* for data transfer - see :doc:`comm_archive`, plus other utility scripts)
* **askap-cray** - Required for building ASKAPsoft, but not for running any of the tools.
* **askapcli** - Command-Line-Interface to the online systems (scheduling block service, FCM and footprint are perhaps the most commonly-used). 
* **casa** - NRAO's CASA software package (see :doc:`casa`)
* **bbcp** - BBCP Fast file copy (see :doc:`externaltransfer`)
* **ashell** - Connection to the commissioning archive (see :doc:`comm_archive`)
* **aces** - A python virtualenv that can used with the ACES tools (see `ACES wiki page`_).
* **acesops** - A tagged version of the ACES subversion repository, designed to give a static set of ACES tools to enable reproducibility of processing.
* **aoflagger** - An alternative flagging tool, written by Andre Offringa. Available as a pipeline option.
  
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

    module load askapsoft/0.19.0

Often it is useful to understand exactly how a module will modify your environment. The
*module display* command can be used to determine this. For instance::

    $ module display askapsoft
    -------------------------------------------------------------------
    /group/askap/modulefiles/askapsoft/0.19.7:

    module-whatis    ASKAPsoft software package 
    prepend-path     PATH /group/askap/askapsoft/0.19.7/bin 
    prepend-path     LD_LIBRARY_PATH /group/askap/askapsoft/0.19.7/lib64 
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
