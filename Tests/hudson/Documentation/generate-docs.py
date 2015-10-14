#!/usr/bin/env python
'''
This program searches through ASKAPsoft for the various locally written
documentation and makes it available through the Redmine 'embedded' plugin.
The embedded plugin has a global root location which for ASKAP Redmine is
  phoenix:/var/www/vhosts/pm.atnf.csiro.au/embedded/askap
For each Redmine project its documentation is in the relative location
  <project id>/html
e.g. for Computing IPT
  cmpt/html 
Below this, the name of the directory specifies the single stylesheet and
single javascript file that is applied to that directory and all its sub
directories.  These have to be configured in Redmine.

The ASKApsoft documentation is produced in Code by running
  python autobuild.py -t doc

To Do:
1. build and install documentation in Tools/Dev
2. make the individual packages form a unified view.

'''

import datetime
import os
import sys
import tempfile

DEBUG = 0

WORKSPACE = os.getenv('WORKSPACE')
TOPDIR    = os.path.join(WORKSPACE, 'trunk')
C_DIRS    = [os.path.join('Code', d) for d in ['Base', 'Interfaces',
                                              'Components', 'Systems']]
T_DIRS    = [os.path.join('Tools', d) for d in ['Dev']]
TARGET    = '/var/www/vhosts/pm.atnf.csiro.au/htdocs/askap-docs/askaptos'
RHOST     = 'phoenix'
TAB       = ' '*4
PRE_SSH   = 'eval `ssh-agent` && ssh-add ${HOME}/.ssh/id_dsa-nokeys'
POST_SSH  = 'kill ${SSH_AGENT_PID}'

TOP_INDEX = ''' <!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN"
"http://www.w3.org/TR/html4/loose.dtd">
<html lang="en-au">

<!-- #BeginTemplate "templates_web/ft-subsite_homepage.dwt" -->

<head>
<!-- SSI: no edit no replace next line -->
<link href="/csirostyles/global_nav.css" rel="stylesheet" type="text/css" media="all">


<!-- ULTIMATE DROP DOWN MENU Version 4.45 by Brothercake -->
<!-- http://www.udm4.com/ -->
<script type="text/javascript" src="/csiroscripts/udm-custom.js"></script>
<script type="text/javascript" src="/csiroscripts/udm-control.js"></script>
<!-- <script type="text/javascript" src="/csirostyles/udm-style.js"></script> -->

<link rel="stylesheet" type="text/css" href="/csirostyles/udm-style.css" media="screen, projection">
<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-1">
<meta http-equiv="Content-Language" content="en-au">
<!-- #BeginEditable "metadata" -->
<meta name="CSIRO.type" content="General">
<meta name="CSIRO.userGroup" content="All">
<meta name="CSIRO.validUntil" content="2012-01-01">
<meta name="DC.creator" content="%%creator%%">
<meta name="DC.date.created" content="%%created%%">
<meta name="DC.date.modified" content="%%modified%%">
<meta name="DC.description" content="%%description%%">
<meta name="DC.format" content="text/html">
<meta name="DC.publisher" content="CSIRO">
<meta name="DC.rights" content="http://www.csiro.au/rights">
<meta name="DC.subject.keyword" content="%%keywords%%">
<meta name="DC.title" content="%%title%%">
<!-- #EndEditable -->

<link href="/csirostyles/base.css" rel="stylesheet" type="text/css">
<link href="/csirostyles/print.css" rel="stylesheet" type="text/css" media="print">


<link href="styles/askapStyle.css" rel="stylesheet" type="text/css" media="all">
<!--<link href="/styles/askapdocsdb.css" rel="stylesheet" type="text/css" media="all">
<link href="/styles/formelements.css" rel="stylesheet" type="text/css" media="all"> -->

<script type="text/javascript" language="javascript" src="/csiroscripts/init.js?rev=ICv7xM5bvv5kFLfurJciEg%3D%3D"></script>
<script type="text/javascript" language="javascript" src="/csiroscripts/core.js?rev=PQia%2B0EyudqxiQyTfOSR%2Fw%3D%3D" defer></script>
<script type="text/javascript" language="javascript" src="/csiroscripts/non_ie.js?rev=yfNry4hY0Gwa%2FPDNGrqXVg%3D%3D"></script>
<script type="text/javascript" language="javascript" src="/csiroscripts/search.js?rev=yqBjpvg%2Foi3KG5XVf%2FStmA%3D%3D" defer></script>

<!-- #BeginEditable "doctitle" -->
<title>ASKAP TOS Documents</title>

<!-- #EndEditable -->
</head>

<body>

<a name="top"></a>
<!-- SSI: no edit no replace next line -->
<!-- scripts required by UDM - these must appear in the body -->
<script type="text/javascript" src="/csiroscripts/udm-dom.js"></script>
<script type="text/javascript" src="/csiroscripts/udm-mod-keyboard.js"></script>

<script type="text/javascript" src="/csiroscripts/validateSearch.js"></script>


<!-- skip links -->
<em class="ign_skiplinks">&#8226; Skip To: <a href="#ign_content">Content</a> | <a href="#ign_nav">Global Navigation</a> | <a href="#ign_subnav">Subsite Navigation</a> | <a href="#ign_search">Search</a></em>

<!-- header bar -->
<table id="ign_header" width="100%" cellpadding="2" cellspacing="0" summary="header-layout" style="margin-bottom:0">

  <tr>
    <td width="1%" id="ign_search_left">
<!--	<a href="http://intranet.csiro.au/intranet/"><b><img src="http://intranet.csiro.au/intranet/global_nav/global_header.gif" alt="CSIRO Intranet" width="300" height="75" border="0" title="Link to CSIRO Intranet Home"></b></a> 
-->
<a href="http://intranet.csiro.au/intranet/index.htm" title="CSIRO Intranet"><img src="/askap-docs/images/csiro_logo.gif" width="50" height="60" border="0"/></a>
</td>
    <td width="85%" align="right" class="ign_hidablex" id="ign_search_right">
<table><tr><td width="100%">&nbsp;</td><td><a href="index.html" title="ASKAP Documentation Home Page"><img src="images/ASKAP_sunup_antennas69x60.jpg" width="69" height="60" border="0"/ alt="ASKAP Doc Home"></a></td></tr></table>
	  </td>
	  
  </tr>
</table>
<!-- end header bar -->

<!-- navigation bar -->
<table id="ign_nav" width="100%" border="0" cellspacing="0" cellpadding="0" style="margin-top:0">
	<tr>
		<td id="ign_nav_udm">
			<h2 class="ign_heading">Global Navigation</h2>

<ul id="udm" class="udm">

<li><a href="http://intranet.csiro.au/intranet/global_nav/quicklinks.htm" title="Quick Links" accesskey="q">Quick Links</a>

<ul>
<li><a href="http://pm.atnf.csiro.au/askap-docs" target="_blank">ASKAP Documentation Home</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP" target="_blank">SharePoint ASKAP</a></li>
<li><a href="https://pm.atnf.csiro.au/askap/projects/att/wiki/ASKAP_Documentation" target="_blank">Document Index</a></li>
<li><a href="https://svn.atnf.csiro.au/askap/" target="_blank">ASKAP Subversion</a></li>
<li><a href="https://pm-int.atnf.csiro.au:8443/contour/">Jama Contour</a></li>					
</ul>
</li>
<li><a href="http://teams.csiro.au/sites/ASKAP" title="SharePoint ASKAP IPTs" accesskey="i">Sharepoint ASKAP IPT Documents</a>
<ul>

<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/ANS"><b>ANS</b> - Analog Systems</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/ANT"><b>ANT</b> - Antennas</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/ARI"><b>ARI</b> - ASKAP Remote Infrastructure</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/CMPT"><b>CMPT</b> - Computing</a></li>

<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/DGS"><b>DGS</b> - Digital Systems</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/DST"><b>DST</b> - Digital and Signal Transport</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/SEIC"><b>SEIC</b> - System Engineering, Integration and Commissioning</a></li>
<li><a href="http://teams.csiro.au/sites/ASKAP/AtT/SUP"><b>SUP</b> - Science User Policy</a></li>
</ul>
</li>
				
<li><a href="http://teams.csiro.au/sites/SearchCentre" title="SharePoint Search" accesskey="s">Search</a>
<ul>
<li><a href="http://teams.csiro.au/sites/SearchCentre">Sharepoint Simple</a>
<li><a href="http://teams.csiro.au/sites/SearchCentre/Advanced.aspx">Sharepoint Advanced</a>
<li><a href="https://pm.atnf.csiro.au/askap/search">ATNF PM Search (Redmine)</a></li>
</ul>
</li>
				
				
</ul>


</td>
<td id="ign_nav_udm">
<!-- %%menus%% -->
</td>        
		<td id="ign_nav_static" class="message">
        <!-- %%message%% %%login%% -->
		</td>
	</tr>
</table> <!-- end navigation bar -->

<!-- start of page container -->
<div id="ign_container">
<div id="ign_pagemast">
  <h1 class="siteheading">ASKAP Documentation<!--%%title%%--></h1>
</div>
<div id="ign_subcontainer">
  <table width="100%" border="0" cellpadding="12" cellspacing="0">
    <tr>
      <td valign="top">
      <div id="ign_content">
        <!-- #BeginEditable "content" -->
<!-- %%frontmatter%% -->
<!-- %%content%% -->

<table cellpadding="4" cellspacing="0" border="0"><tr><td>
<img src="images/iconProject32x39.png">
</td>
<td>
<h1>TOS Documentation</h1>
</td></tr></table>

This documentation is generated by a Jenkins job every night.
<ul>
  <li> <a href="./doxygen">doxygen</a> 
  <li> <a href="./ice">ice</a> 
  <li> <a href="./javadocs">javadocs</a> 
  <li> <a href="./sphinx">sphinx</a> 
</ul>

  
</div>
<!-- SSI: no edit no replace next line -->
</div>
<!-- end of page container -->

<div id="ign_footer">
  
	<div id="ign_footer_nav">
		<p>
			[ <a href="http://intranet.csiro.au/intranet/index.htm">CSIRO Intranet</a> ]&nbsp;
<!--			[ <a href="http://intranet.csiro.au/intranet/webadmin/contact_us.htm">Contact Us</a> ]&nbsp; 
			[ <a href="http://intranet.csiro.au/intranet/webadmin/site_index.htm">Site Map</a> ]&nbsp; -->

			[ <a href="http://www.csiro.au/org/CopyrightNotice.html">Copyright</a> ]&nbsp;
			[ <a href="http://www.csiro.au/org/LegalNoticeAndDisclaimer.html">Disclaimer</a> ]

		</p>
	</div> <!-- end global footer nav -->

	<p id="ign_copyright">

		&copy; Copyright 1994-2015, CSIRO Australia. <a href="http://www.csiro.au/org/Privacy.html">Privacy Statement</a>.
	</p>
<!-- end global copyright -->
  
</div> <!-- end global footer -->



</body>

<!-- #EndTemplate -->

</html>
'''

INDEX_HEAD = '''<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN"
        "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">

<html xmlns="http://www.w3.org/1999/xhtml" lang="en" xml:lang="en">

  <head>
    <title>Welcome to pm.atnf.CSIRO.AU</title>
  </head>

  <body>
    <h2> ASKAP TOS %s Documents </h2>
    <p>
    The documentation in this area is %s auto-generated.
    </p>

    <h3> Packages </h3>
'''

INDEX_TAIL = '''
    <p>
    <i>Last modified: %s.</i>
    <br/>
    &copy; Copyright CSIRO Australia %s.
    <a href="http://www.csiro.au/index.asp?type=aboutCSIRO&amp;xml=mrcopyright&amp;style=generic">CSIRO Media Release information</a>,
    <a href="http://www.csiro.au/index.asp?type=aboutCSIRO&amp;xml=disclaimer&amp;style=generic">Legal Notice and Disclaimer</a>,
    <a href="http://www.csiro.au/index.asp?type=aboutCSIRO&amp;xml=privacy&amp;stylesheet=generic">Privacy Statement</a>.
    </p>

  </body>
</html>
'''


def BuildTree(path, partial_paths):
    '''
    Given a package directory path, return list of tuples containing indent
    level and directory name of partial paths.
    Need to keep track of partial paths already seen.
    '''

    path_components = path.split('/')
    p = ''
    items = []

    for comp in path_components[0:-1]:
        p += comp + '/'
        if not p in partial_paths:
            partial_paths.append(p)
            items.append((len(p.split('/')), "<li> %s </li>" % comp))
    return items


def CopyDocs(doc_type, doc_loc, find_term):
    '''
    The ASKAPsoft system uses different type of documentation depending on the
    language used. e.g. java -> javadocs, python -> sphinx, C++ -> doxygen
    This is the "doc_type" parameter.
    The documentation is produced in different locations depending
    on the type.  This is the "doc_loc" parameter.
    To double check we are getting the correct documentation (and avoid any
    third party documentation) search for a particular file. This is the
    "find_term" parameter.

    This function does two things.
    1. Creates a document type specific index file that is copied to the
       remote machine in the top level document type directory.
       e.g. cmpt/html/[doxygen|javadocs|sphinx]
       This contains an unordered list <ul> which points to the
       subdirectories containing documentation.
    2. Copies the ASKAPsoft documentation to the remote machine rooted in the
       top level document type directory, keeping the relative path from the
       build system down to the package level i.e. the 'doc_loc' is stripped
       from the end of the path.
    '''

    indent = 0
    orig_indent = 2
    old_indent = orig_indent
    partial_paths = []
    items = []
    index = INDEX_HEAD % (doc_type.capitalize(), doc_type)
    index += TAB*orig_indent + "<ul>\n" # Start indented under '<h3> Packages </h3>'

    # os.popen() is a file like object which contains a list of paths to the
    # package level which should contain documentation of specified type. e.g.
    # /tmp/ASKAPsoft/Code/Base/py-loghandlers
    # /tmp/ASKAPsoft/Code/Base/py-parset

    PKGS = []
    sed_term = "/%s/%s" % (doc_loc, find_term)
    for subdir in T_DIRS + C_DIRS:
        fdir = os.path.join(TOPDIR, subdir)
        cmd = 'find %s -name %s | sed "s#%s##"' % (fdir, find_term, sed_term)
        if DEBUG:
            print '     ', cmd
        PKGS += os.popen(cmd).readlines()

    for pkgpath in PKGS:
        pkgpath = pkgpath.strip() # Needed for next line to work correctly.
        pkg = pkgpath.split('/')[-1] # Last bit is package name.
        subtree = pkgpath.split(TOPDIR + os.sep)[-1] # i.e. relative path
        source = "%s/%s/" % (pkgpath, doc_loc) # Recreate the full path.
        target = "%s/%s/%s" % (TARGET, doc_type, subtree)
        print "info: %s" % pkg

        if os.path.isdir(source): # It should be a directory.
            items += BuildTree(subtree, partial_paths)
            indent = len(subtree.split('/')) + 1
            items += [(indent, '<li><a href="%s">%s</a></li>' % (subtree, pkg))]
            if DEBUG:
                print "      src = %s" % source
                print "      dst = %s" % target
            else:
                os.system("%s && ssh %s mkdir -p %s; %s" %
                          (PRE_SSH, RHOST, target, POST_SSH))
                #os.system("%s && rsync -av --delete %s %s:%s; %s" %
                os.system("%s && rsync -av %s %s:%s; %s" %
                          (PRE_SSH, source, RHOST, target, POST_SSH))

    # Take the list of item tuples and convert it to string while adding the
    # necessary HTML unordered list markup codes.
    for item in items:
        if item: # ignore any 'None' items.
            indent, text = item
            if indent < old_indent:
                for i in reversed(range(indent, old_indent)):
                    index += TAB*(i+1) + "</ul>\n"
            elif indent > old_indent:
                index += TAB*indent + "<ul>\n"
            index += TAB*indent + text + "\n"

            old_indent = indent

    # Finish off all outstanding unordered lists <ul>.
    # The number outstanding can be determined from the current indent level
    # remember the default indent level for the list is orig_indent.
    for i in reversed(range(orig_indent, indent+1)):
        index += TAB*i + "</ul>\n"

    # Add the tail to the index file and write out so that it can
    # be scp to remote destination.
    index += INDEX_TAIL % (today.strftime("%d %b %Y"), today.year)
    if DEBUG:
        print '\n\n', index
    else:
        f = tempfile.NamedTemporaryFile()
        f.write(index)
        f.flush()
        os.chmod(f.name, 0444)
        os.system("%s && scp %s %s:%s/%s/index.html; %s" %
                  (PRE_SSH, f.name, RHOST, TARGET, doc_type, POST_SSH))
        f.close()


def CopyIndex():
    f = tempfile.NamedTemporaryFile()
    f.write(TOP_INDEX)
    f.flush()
    os.chmod(f.name, 0444)
    os.system("%s && scp %s %s:%s/index.html; %s" %
        (PRE_SSH, f.name, RHOST, TARGET, POST_SSH))
    f.close()


#
# Main Program
#

today = datetime.date.today()

print "info: Doxygen"
CopyDocs("doxygen", "html", "doxygen.css")
print "info: Javadocs"
CopyDocs("javadocs", "doc", "stylesheet.css")
print "info: Sphinx"
CopyDocs("sphinx", "doc/_build/html", "objects.inv")
print "info: Ice"
CopyDocs("ice", "slice/current/doc", "_sindex.html")

CopyIndex()
