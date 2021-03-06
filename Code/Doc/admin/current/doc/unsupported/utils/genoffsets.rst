genoffsets
============

Usage::

    genoffsets.sh genoffsets.parset

This utility computes offsets defining a beam arrangement and applies them to
a given reference direction to obtain pointing directions positioning reference
direction into each beam. This tool is used to ::
  
   * Form a set of pointings to place the Sun in the right place to obtain ACMs for
     beamforming (i.e. to obtain weights for the beam in a desired location)
   * Form a set of pointings to put a calibrator in each beam

The results are written into standard output at the moment. We may add functionality
in the future to write a file with the format appropriate for the scheduling block
creation. Currently this step (or pointing in an old way used prior to scheduling
blocks) done with python scripts parsing the captured output of this tool.
The main motivation to do these calculations in a C++ tool is because pyephem lacks
some essential functionality to do the required calculations in a generic way which
works with equal accuracy everywhere in the sky.

Parset file defines the desired beam layout which may be composed from any combination
of the following ::

    * explicitly defined offsets
    * offsets corresponding to that between two positions on the sky, i.e if we point
      the dish to one position, the beam will point to the other
    * as above, but with some scaling factor allowing to generate one or more beams
      on the line determined by two sky coordinates. In particular, this allows
      to put a beam half way between the boresight and a beam generated by the previous
      method or on the opposite side symmetrically with respect to the boresight
      direction
    * complement two already defined beams with a third one forming an equilateral 
      triangle with them. Note, one could not refer to beams defined later (i.e. with
      a higher sequence number).
    

Note, all sky coordinates are required to be given in CASA format at the moment, i.e.
degrees, minutes and seconds of the declination should be separated by a dot, 
rather than a semicolon. The same reference frame is assumed for all coordinates and
the output is in the same frame (i.e. it is J2000 if the results are fed directly into
the online system or a scheduling block).

The parset parameters are described in the following table:

+------------------------------+--------------+--------------------+-----------------------------------------+
|*Parameter*                   |*Type*        |*Default*           |*Description*                            |
+==============================+==============+====================+=========================================+
|sources                       |vector<string>|empty vector        |Names of explicitly defined positions on |
|                              |              |                    |the sky                                  |
+------------------------------+--------------+--------------------+-----------------------------------------+
|sources.name                  |vector<string>|none                |For each name listed in **sources** there|
|                              |              |                    |should be a source.name key defining     |
|                              |              |                    |position on the sky (as a two-element    |
|                              |              |                    |vector with RA and Dec, see the example) |
+------------------------------+--------------+--------------------+-----------------------------------------+
|nbeams                        |int           |none                |Number of beams in this arrangement. It  |
|                              |              |                    |defines the number of rules which must be|
|                              |              |                    |present in the parset file.              |
+------------------------------+--------------+--------------------+-----------------------------------------+
|beamN                         |string        |none                |Each of nbeams beams must be defined with|
|                              |              |                    |one of the supported rules. This keyword,|
|                              |              |                    |where N is an integer number from 1 to   |
|                              |              |                    |nbeams, inclusive, defines the rule for  |
|                              |              |                    |beam number N                            |
+------------------------------+--------------+--------------------+-----------------------------------------+
|refdir                        |vector<string>|Sun's direction     |Reference direction used to generate     |
|                              |              |                    |offsets, i.e. if one points the dish to  |
|                              |              |                    |the position generated by this tool for  |
|                              |              |                    |some beam, this beam will point to the   |
|                              |              |                    |direction specified in this parameter. By|
|                              |              |                    |default, it equates to the Sun's position|
|                              |              |                    |at MRO for the current time and therefore|
|                              |              |                    |this tool forms a position to point the  |
|                              |              |                    |dish to in order to capture ACMs for     |
|                              |              |                    |appropriate beamforming. If one puts the |
|                              |              |                    |position of 1934-638 here, the tool will |
|                              |              |                    |calculate the poiting positions for      |
|                              |              |                    |calibration.                             |
+------------------------------+--------------+--------------------+-----------------------------------------+


Rules to define beams
---------------------

..code-block::
    beamN = offset(X,Y) 

defines beam N at explicit offset (X,Y) degrees from the boresight

..code-block::
    beamN = F(name1,name2) 

defines beam N on the line corresponding to the offset between directions name1 and name2 
(which should be listed in the **source** keyword). Exact position is determined by the 
floating point factor F. This factor could be omitted, in which case it is assumed to be +1.
Imagine the dish points to the direction represented by name2. Then, in the case of F=0, the
beam defined by this rule will also point to name2. If F=+1 (default), this beam will point
to name1. If F=+0.5, the beam will point half way between name1 and name2. And so on. Note,
that the factor F could be negative. In particular, if F=-1, the beam defined by this rule
will point to a direction located on the opposite side of name2, away from name1 but at
the same angular separation as name1 is offset from name2.

..code-block::
    beamN = triangle(beam1,beam2,which) 

defines beam N by complementing two existing beams, beam1 and beam2 given by their 
1-based indices, to form an equilateral triangles. There are two such triangles available,
which are differentiated by the third parameter (labelled "which"). The recognised values
are left and right, which corresponds to the direction the third vertex of the triangle is
seen at if one is at beam2 and looks towards beam1. In the non-degenerate case, "left" 
corresponds to a larger right ascension. 

Examples
--------

**Example 1:**

This is the parset which generates a beam arrangement used in the early 9-beam experiments with the
circumpolar "Cluster" field. It has two beams (1 and 3) pointing on two bright sources in the field
and the rest are build around them.

..code-block:: bash

    # define two positions on the sky
    sources = [ref,src1]

    # we point at this position during observations of this field
    # note, the CASA-style of the declination
    sources.ref = [15h56m58.871,-79.14.04.28]
    # this is the position of another bright source we want in beam 3
    sources.src1 = [16h17m49.278,-77.17.18.46]

    # we have 9 beams to describe 
    nbeams = 9

    # description of each beam (parset key name is 1-based beam number)

    # first beam is just the boresight direction - define it via the explicit offsets
    beam1 = offset(0,0)

    # beam 2 is half way between two bright sources
    beam2 = 0.5(src1,ref)

    # beam 3 points to src1 if the dish points to ref
    # note, no number before bracket is equivalent to +1
    beam3 = (src1, ref)

    # half way between src1 and ref, but reflected w.r.t. ref
    beam4 = -0.5(src1, ref)

    # the following options complement a triangle based on already defined beams
    # Note, can only refer to beams with lower numbers!

    # beam5 is at a  missing vertex of the equilateral triangle formed by beam1 and beam2
    # the option with higher ra is selected for this one (hence, left)
    beam5 = triangle(1,2,left)

    beam6 = triangle(2,3,left)

    # same as beam5, but flipped the opposite way
    beam7 = triangle(1,2,right)

    beam8 = triangle(2,3,right)

    beam9 = triangle(4,1,right)

