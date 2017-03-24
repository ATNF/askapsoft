External Data Transfer
======================

You will often want to transfer data between the ASKAP Central Processor and computers
at your home institution. For example, upon completion of imaging you may want to visualise
those images on your desktop computer.

You may use the secure copy program (scp) to copy data to/from either your home directory or
the scratch filesysyem. A special data-mover node exists, with hostname *hpc-data.pawsey.org.au*,
for this purpose. It is possible to copy data through the login nodes, however this should
be avoided if possible so as to reduce the load on the login
nodes. This data-mover node is actually a front-end to one of four
nodes, and these can see both /scratch2 (on galaxy) and /scratch (on magnus).

Here is an example of copying a file from both the home directory and the scratch filesystem.
Note these commands are executed on your local host (e.g. your desktop or laptop)::

    scp -r hpc-data.pawsey.org.au:/scratch2/askap/user123/image.fits .
    scp -r hpc-data.pawsey.org.au:~/1934-638.ms .

or to copy from your laptop/desktop to the Central Processor::

    scp -r image.fits hpc-data.pawsey.org.au:/scratch2/askap/user123
    scp -r 1934-638.ms hpc-data.pawsey.org.au:~

Note the "-r" performs a recursive copy so can be used to copy files or directories.

Using BBCP
----------

Using *scp* can be quite slow and a program called *bbcp* is suggested for large files.

* Binary packages: http://www.slac.stanford.edu/~abh/bbcp/bin/
* Obtaining the source with git: "git clone http://www.slac.stanford.edu/~abh/bbcp/bbcp.git"
* Command line parameters: http://www.slac.stanford.edu/~abh/bbcp/

The ASKAP software team can supply Debian packages. Usage is similar to scp, but with
a few extra parameters. To copy a file from the /scratch2 filesystem::

    bbcp -z -P 10 -s 32 -w 2M -r hpc-data1.pawsey.org.au:/scratch2/askap/user123/image.fits .

and to copy a file to the /scratch2 filesystem::

    bbcp -P 10 -s 32 -w 2M -m 2755/644 -r image.fits hpc-data1.pawsey.org.au:/scratch2/askap/user123

.. note:: The hostname necessary to use bbcp is *hpc-data1.pawsey.org.au*. This is one of the
          four hosts to which the *hpc-data* DNS alias points to (the
          other that works is *hpc-data2*).
          This is necessary as bbcp doesn't reliably establish connections via the galaxydata
          alias due to the fact connections are round-robined between its four nodes.

The three additional options result in production of progress messages every 10 seconds,
sets the number of parallel network streams to be used for the transfer to 32, and sets the
preferred size of the TCP window to 2MB. On some networks increasing the number of streams
from 32 (up to a maximum of 64) may result in even higher throughput.

