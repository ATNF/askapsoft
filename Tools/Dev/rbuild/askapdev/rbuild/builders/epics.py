from __future__ import absolute_import
from builtins import str
## @file
# Object to simplify unpacking/building/cleaning of ASKAPsoft packages
#
# @copyright (c) 2007-2013 CSIRO
# Australia Telescope National Facility (ATNF)
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# PO Box 76, Epping NSW 1710, Australia
# atnf-enquiries@csiro.au
#
# This file is part of the ASKAP software distribution.
#
# The ASKAP software distribution is free software: you can redistribute it
# and/or modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of the License
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA.
#
# @author Juan Carlos Guzman <Juan.Guzman@csiro.au>
#
import os
import shutil
import sys
import glob
import csv
import re

from .builder import Builder
import askapdev.rbuild.utils as utils
from ..exceptions import BuildError
from ..dependencies import Dependency


## Implementation of Builder for EPICS applications.
# It overwrites _precommand, _clean, _postcommand.
class Epics(Builder):
    ## The constructor sets up a package build "environment"
    #  @param self           The current object
    #  @param pkgname the name of the package directory
    #  By default it is the basename of the current directory.
    #  For EPICS applications (in the Code tree) use pkgname '.'.
    #  @param archivename The tarball name.
    #  Specify this if tarball name differs from pkgname
    #  (ignoring the .tar.(gz|bz2) suffix).
    #  @param extractdir The name of directory to be created and extract
    #  tarball into this directory.
    #  @param epicsbase the parameter name for epicsbase in
    #  dependencies.default file.  Usuallly "epicsbase".
    #  @param releasefile The name of release file to be copied into the
    #  @param supportmodule True if an EPICS support module
    #         (for package name/logging)
    #  configure directory.
    #  By default is RELEASE.<epics architecture>
    def __init__(self, pkgname=None, archivename=None, extractdir=None,
                 epicsbase=None, releasefile=None, supportmodule=False):
        Builder.__init__(self, pkgname=pkgname,
                         archivename=archivename,
                         extractdir=extractdir,
                         buildcommand="make",
                         buildtargets=[""])
        self.parallel = False
        self._epicssupportdict = {}  # support modules
        self._iocbootdirsdict = {}  # iocboot directories to install
        self._implicit_deps = []
        self._support_module = supportmodule
        self.package_version = "0.0"

        if epicsbase is None:
            # We are building EPICS base (no need for RELEASE file)
            self._epicsbase = os.path.join(os.path.abspath(os.curdir),
                                           self._installdir)
            self._is_epics_base = True
        else:
            self._epicsbase = self.dep.get_install_path(epicsbase)
            self._is_epics_base = False
            self._appname = os.getcwd().split(os.sep)[-2]

        self._epicsbase = self._epicsbase.rstrip(os.sep)

        # Set EPICS architecture using shell script in EPICS base directory.
        comm = os.path.join(os.path.dirname(self._epicsbase), "EpicsHostArch")
        self._epicsarch = str(utils.runcmd(comm)[0].strip().decode())
        self.add_env("EPICS_HOST_ARCH", self._epicsarch)
        self._oldreleasefile = None

        if releasefile is None:
            self._releasefile = "RELEASE." + self._epicsarch + ".Common"
            # Create old (<3.14.11) release file for compatibility with
            # old style of CONFIG files that uses CONFIG_APP, for example
            # asyn, modbus
            self._oldreleasefile = 'RELEASE.' + self._epicsarch
            self.add_extra_clean_targets(self._oldreleasefile,
                                         os.path.join('configure',
                                                      self._oldreleasefile))
        else:
            self._releasefile = releasefile
        self.add_extra_clean_targets(self._releasefile,
                                     "package.info",
                                     os.path.join('configure',
                                                  self._releasefile))
        self._deps_file = os.path.join(self._bdir, "configure",
                                       "ASKAPDEPS")
        self.add_extra_clean_targets(self._deps_file)

        ldname = "LD_LIBRARY_PATH"
        if sys.platform == "darwin":
            ldname = "DY" + ldname
        if ldname in os.environ:
            os.environ[ldname] = \
                os.path.pathsep.join([self.dep.get_ld_library_path(),
                                      os.environ[ldname]])
        else:
            os.environ[ldname] = self.dep.get_ld_library_path()
        os.environ["PATH"] = os.path.pathsep.join([self.dep.get_path(),
                                                   os.environ["PATH"]])
        # Setup version header file name if not epics base
        if not self._is_epics_base:
            self._version_header_filename = self._appname + '_version.h'
            self.add_extra_clean_targets(self._version_header_filename)

        self._csv_files = []

    ## Add EPICS support module needed to compile EPICS application.
    #  Support list is included in the auto-generated release file.
    #  @param module  The name of the module (example 'asyn')
    #  @param path    The absolute path where module is located
    #
    #  The module name shoud match the modules parameter name in the
    #  dependencies.default file.
    # Allow the pathname to be specified. XXX Cannot think why this should be
    # needed.
    def add_support(self, module, path=None, auto=False, appname=None):
        if not auto:
            if path:
                self._epicssupportdict[module] = path
            else:
                self._epicssupportdict[module] = \
                    self.dep.get_install_path(module)
            return
        if not utils.in_code_tree():
            return
        if appname is None:
            appname = self._appname
        pth = self.dep.get_dep_path(module)
        if pth.startswith("/"):
            libs = [(l, None) for l in self.dep.get(module)["libs"]]
        else:
            dep = Dependency()
            dep.add_package(module, pth)
            libs = dep.get_libs(mapped=True)
            #rel_libs = [(m, p) for m, p in libs]
        # libs = self.dep.get_libs_for(module, mapped=True)
        tgt = []
        # remove 'module' as the that is added explicitly above
        for lib, libdir in libs:
            # Namespace these so there are no conflicts,
            # e.g. berkley 'db' and epcis internal 'db'
            name = 'ASKAPDEP_' + lib
            # For some reason we have to add the directories to the
            # RELEASE file and then use the variables?
            # Explicit paths in the Makefile won't work.
            #print(appname, libdir, lib)
            if libdir:
                if lib not in [i[1] for i in self._implicit_deps if
                               i[2] == appname]:
                    self._epicssupportdict[name] = libdir
                    tgt.append((name, lib, appname))
            else:
                if lib not in [i[1] for i in self._implicit_deps if
                               i[2] == appname]:
                    tgt.append((name, lib, appname))
        self._implicit_deps += tgt

    def _create_askapdefs(self):
        if not self._implicit_deps:
            return
        with open(self._deps_file, 'w') as f:
            for name, lib, appname in self._implicit_deps:
                if name in self._epicssupportdict:
                    libs = appname + "_LIBS += "
                    f.write(libs + lib + "\n")
                    f.write("%s_DIR += $(%s)/lib\n" % (lib, name.upper()))
                else:
                    f.write("%s_SYS_LIBS += %s\n" % (appname, lib))

    ## Add EPICS iocboot area to be installed in install dir during
    #  post_command()  execution. If application name is not defined,
    #  it gets extracted from the directory name.
    #  Format of directory name should be ioc<appname> and it should
    #  contain a st.cmd.template file,  which will be used to construct
    #  the st.cmd file. The resulting directory in install dir will be
    #  the one passed as iocbootdir argument.
    #  @param srcdir  iocboot directory (where st.cmd.template and
    #                 other files are located)
    #  @param appname Application name to be invoked in the st.cmd
    #                 (default is extracted from directory name)
    def add_install_iocboot(self, srcdir, appname=None):
        srcdir = os.path.normpath(srcdir)  # remove trailing "/" chars
        basename = os.path.basename(srcdir)

        if appname is None:
            if basename.startswith('ioc'):
                appname = basename[3:]  # Extract all the rest after 'ioc'
                if appname == '':
                    utils.q_print('warn: IOC boot directory name is ioc and '
                                  'should be ioc<appname>')
                    utils.q_print('warn: please enter appname in '
                                  'add_install_iocboot() method')
                    appname = '@@@error: no appname@@@'
            else:
                appname = basename

        if os.path.isdir(srcdir):
            # Check that basename does not already exist in dictionary.
            for sdir in list(self._iocbootdirsdict.keys()):
                if basename == os.path.basename(sdir):
                    raise BuildError("IOC boot base directory name already "
                                     "exist")
            self._iocbootdirsdict[srcdir] = appname
        else:
            utils.q_print('warn: iocbootdir >%s< is not a directory.' % srcdir)

    def _epicsbase_configure_exists(self):
        epics_config_dir = os.path.join(self._epicsbase, 'configure')
        return os.path.exists(epics_config_dir)

    ## Create and populate EPICS release file.
    def _create_releasefile(self):
        with open(self._releasefile, "w") as rfh:
            rfh.write("ASKAP_ROOT=%s\n" % self._askaproot)
            rfh.write("EPICS_BASE=%s\n" % self._epicsbase)
            for key, value in list(self._epicssupportdict.items()):
                key = key.replace("-","_")
                rfh.write("{0}={1}\n".format(key.upper(), value))
            if self.package_version is not None:
                rfh.write("SHRLIB_VERSION=%s\n" % self.package_version)
            rfh.write("LINKER_USE_RPATH=NO\nUSE_RPATH=NO\n")

    ## Get all dependencies as single string separated by ';'
    def _get_dependencies(self):
        out = ["=".join([k, v['path']]) for k, v in list(self.dep._deps.items())]
        return ";".join(out)

    ## Update <appname>App/src/<appname>Version.c file with version and 
    # dependencies information
    def _update_version_file(self):
        # Create a new version header file
        version_text = """/*  This is an automatically generated file.
 *  Please DO NOT edit nor add it to the repository
 *  @file
 *
 *  Package version file.
 *  ONLY include in ".c" and ".cc" files never in header files!
 */
#ifndef {app}_H
#define {app}_H

/*
 * The package name for IOC logging
 */
#define ASKAP_PACKAGE_NAME "{prefix}{name}"

/* 
 * The version of the package
 */
#define {app}_PACKAGE_VERSION "{vers}"
#define {app}_PACKAGE_DEPENDENCIES "{deps}"
#endif
""".format(app=self._appname.upper(), name=self._appname,
           vers=self.version_string,
           deps=self._get_dependencies(),
           prefix="" if self._support_module else "ioc.")

        # Ticket 4992: don't update timestamp of generated header file
        updateheader = True
        if os.path.exists(self._version_header_filename):
            with open(self._version_header_filename, 'r') as f:
                txt = f.read()
                updateheader = version_text.strip() != txt.strip()
        if updateheader:
            with open(self._version_header_filename, 'w') as f:
                f.write(version_text)

    ## Copy RELEASE.epicsarch file into configure directory
    def _precommand(self):
        Builder._precommand(self)
        if os.path.exists(os.path.join(self._package, "pvaSrv")):
            # epicsV4 needs to know where EPICS v3 base is
            os.environ["EPICS_BASE"] = self._epicsbase
        elif not self._is_epics_base:
            self._create_releasefile()
            self._create_askapdefs()
            dstdir = os.path.join(self._package, "configure")
            if not os.path.exists(dstdir):
                os.mkdir(dstdir)
            shutil.copy(self._releasefile, dstdir)
            if self._oldreleasefile is not None:
                shutil.copy(self._releasefile,
                            os.path.join(dstdir, self._oldreleasefile))
            self._update_version_file()

    def _doc(self):
        if utils.in_code_tree() and os.path.exists('setup.py'):
            env = self._get_env()
            cmd = "%s python setup.py doc" % (env,)
            utils.run("%s" % cmd, self.nowarnings)

    def _clean(self):
        if utils.in_code_tree() and os.path.exists('setup.py'):
            utils.run("python setup.py clean")

        # Bug #2803
        # An ASKAP/EPICS application (pkgname == '.') usually has a configure 
        # directory in the root directory
        # as opposed to EPICS base and some support modules where the tarball 
        # gets expanded in the pkgname directory.
        # This feature affects the way the package gets cleaned in order to 
        # support idempotent cleaning command.
        # In case of ASKAP/EPICS applications, we need to check whether EPICS 
        # base configure directory exists, otherwise
        # we cannot execute 'make clean' command. If epics base configure 
        # directory exists, a RELEASE.<architecture> file
        # must exist in the configure directory in order to locate epics base 
        # configure directory for the make command
        # to work correctly.
        if self._package == '.':
            if self._epicsbase_configure_exists():
                # RELEASE.<arch> must exists in order to run make clean. 
                # This prevents an error when running clean when the package 
                # has already been cleaned.
                self._create_releasefile()
                if not os.path.exists(self._deps_file):
                    open(self._deps_file, 'w').write("")
                shutil.copy(self._releasefile,
                            os.path.join(self._package, "configure"))
                if self._oldreleasefile is not None:
                    shutil.copy(self._releasefile,
                                os.path.join(self._package,
                                             "configure", self._oldreleasefile))
                curdir = os.path.abspath(os.curdir)
                # Enter the untarred package directory
                os.chdir(self._package)
                utils.run("make clean uninstall")
                os.chdir(curdir)
            else:
                utils.q_print("WARNING: EPICS base configure directory does "
                              "not exists (required by 'make clean'). "
                              "Some temporary files inside the package will not"
                              "be removed. Build EPICS base and re-run clean "
                              "target or delete temporary files manually.")
        # Execute base class method, which removes install directory and 
        # additional clean targets
        Builder._clean(self)

    def _create_info(self):
        entries = {}
        libdir = os.path.join(self._prefix, "lib", self._epicsarch)
        rx = r"^lib(.+)\.so$"
        if os.path.exists(libdir):
            entries["libdir"] = os.path.join("lib", self._epicsarch)
            libs = [p.split("/")[-1] for p in glob.glob(libdir+"/*.so")]
            libs = [re.findall(rx, p)[0] for p in libs]
            entries["libs"] = " ".join(libs)
        else:
            entries["libdir"] = ""
            entries["libs"] = ""
        if os.path.exists(os.path.join(self._prefix, "include")):
            entries["incdir"] = "include"
        else:
            entries["incdir"] = ""
        if os.path.exists(os.path.join(self._prefix, "bin")):
            entries["bindir"] = "bin/" + self._epicsarch
        else:
            entries["bindir"] = ""

        inf = "package.info"
        # read in explicit entries
        inlines = []
        if os.path.exists(inf + ".in"):
            inlines = open(inf + ".in", "r").readlines()

        for l in inlines:
            l = l.strip()
            if l and not l.startswith("#"):
                k, v = l.split("=", 1)
                entries[k] = v

        with open(inf, "w") as pih:
            pih.write("# Auto-generated by build.py\n")
            for k, v in list(entries.items()):
                pih.write("{0}={1}\n".format(k, v))

            ##? This doesn't seem to be necessary?
            pih.write("env=EPICS_HOST_ARCH=%s\n" % self._epicsarch)
            extraincdir = os.path.join(self._epicsbase, 'include', 'os',
                                       os.uname()[0])
            pih.write("defs=-I%s" % extraincdir)

    def add_ioc_config(self, envfile=None, hostfile=None, iocname=None):
        """Add deployment configuration files for tunning the ioc.
        These are two csv files (<iocname>_env.csv and <iocname>_host.csv)
        containing individual ioc environment and which IOCs to run on 
        which host. These get installed into install/ioc-config"""
        if not utils.in_code_tree():
            utils.q_print("warn: 'add_ioc_config' only works in Code tree")
            return
        if iocname is None:
            if self._appname.startswith("ioc"):
                iocname = self._appname
            else:
                iocname = "ioc" + self._appname
        if envfile is None:
            envfile = os.path.join("files", iocname + "_env.csv")
        if not os.path.exists(envfile):
            raise IOError("'%s' doesn't exist" % envfile)
        if hostfile is None:
            hostfile = os.path.join("files", iocname + "_host.csv")
        if not os.path.exists(hostfile):
            raise IOError("'%s' doesn't exist" % hostfile)
        self._csv_files.append((envfile, hostfile, iocname))

    def _create_ioc_config(self):
        if not self._csv_files:
            return
        outdir = os.path.join(self._prefix, "ioc-config")
        if not os.path.exists(outdir):
            os.makedirs(outdir)
            self.add_extra_clean_targets(outdir)
        for (envfile, hostfile, iocname) in self._csv_files:
            with open(envfile) as f:
                reader = csv.DictReader(f, skipinitialspace=True)
                for row in reader:
                    suffix = row["SITE"].strip()
                    if "ANTID" in row:
                        antid = row["ANTID"].strip()
                        if antid:
                            suffix += "%02d" % (int(antid))
                    if "SUBSYSTEMNO" in row:
                        sid = row["SUBSYSTEMNO"].strip()
                        if sid:
                            suffix += "_%s" % (sid)
                    with open("{0}/{1}_{2}".format(outdir, iocname,
                                                   suffix), "w") as of:
                        of.write("##Auto-generated {0} config file\n"
                                 .format(iocname))
                        of.write(
                            'test -f /etc/askap/site/arrayenv.sh && . /etc/askap/site/arrayenv.sh\n')
                        of.write('IOC_ROOT="$ASKAP_ROOT"\n')
                        exports = ['IOC_ROOT', ]

                        for k, v in sorted(row.items()):
                            if v == None:
                                raise RuntimeError(
                                    "bad key {0} in row {1}".format(k, row))
                            v = v.strip()
                            if v:
                                outstr = '{}="{}"'.format(k, v)
                                # outstr = "=".join((k, v))
                                of.write(outstr + "\n")
                                exports.append(k)
                        of.write("export %s\n" % " ".join(exports))
            with open(hostfile) as f:
                reader = csv.DictReader(f, skipinitialspace=True)
                for row in reader:
                    with open("{0}/{1}.{2}".format(outdir, iocname,
                                                   row["HOST"].strip()
                                                   ), "w") as of:
                        of.write("##Auto-generated {0} config file\n"
                                 .format(iocname))
                        of.write('test -f /etc/askap/site/arrayenv.sh &&'
                                 ' . /etc/askap/site/arrayenv.sh || true\n')
                        of.write('IOCS="%s"\n' % row["IOCS"].strip())

    def _create_monica_config(self):
        mondir = os.path.join("files", "monica")
        if not os.path.exists(mondir):
            return

        def create_out(d):
            relpath = d.split(os.path.sep, 1)[-1]
            outd = os.path.join(self._installdir, relpath)
            if not os.path.exists(outd):
                os.makedirs(outd)
            return outd

        for d, i, files in os.walk(mondir):
            if d.find('.svn') == -1:
                if d == mondir:
                    continue
                template = [tf for tf in files if tf.endswith(".template")]
                txt = [tf for tf in files if tf.endswith(".txt")]
                if not template:
                    if not txt:
                        continue
                    outd = create_out(d)
                    for t in txt:
                        shutil.copy(os.path.join(d, t), outd)
                    continue
                tf = os.path.join(d, template[0])
                outd = create_out(d)
                of = os.path.join(outd, os.path.splitext(template[0])[0])
                # find any monica include directories
                include = "-I{0} -I{1} ".format(mondir, d)
                for installDir in list(self._epicssupportdict.values()):
                    installDir = os.path.join(installDir, "monica", "include")
                    if os.path.exists(installDir):
                        include += "-I{0} ".format(installDir)
                utils.run("msi -V {0} -o{1} {2}".format(include, of, tf))

    def _install(self):
        # Install iocboot directories from table
        Builder._install(self)
        for ioctemplatedir, appname in list(self._iocbootdirsdict.items()):
            self._install_iocboot(appname, ioctemplatedir)
        self._create_ioc_config()
        self._create_monica_config()
        if not self._is_epics_base:
            pth = os.path.join(self._prefix, "configure", "RELEASE")
            if os.path.exists(pth):
                os.remove(pth)

    ## Create iocBoot directory structure inside install dir and startup
    #  script based on templates.
    #  This method adds the following lines at the beginning of the
    #  startup script template:
    #    #!../bin/<arch>/<appname>
    #    epicsEnvSet("ARCH", "<arch>")
    #    epicsEnvSet("IOC", "ioc<appname>")
    #    epicsEnvSet("TOP", "../..")
    #    cd ${TOP}
    #  It assumes that the application and all needed libraries are
    #  located inside install dir.
    def _install_iocboot(self, appname, ioctemplatedir):
        # iocdir = 'ioc' + appname
        # jcg: name of the ioc dir should be the name of the ioctemplatedir 
        # instead of appname
        #      to support multiple ioc dirs with same appname
        iocdir = os.path.basename(ioctemplatedir)
        iocbootdir = os.path.join(self._installdir, 'iocBoot', iocdir)
        # Create list of auto-generated lines
        autogenlines = list()
        autogenlines.append('#!/usr/bin/env %s\n' % appname)
        autogenlines.append('epicsEnvSet(\"ARCH\", %s)\n' % self._epicsarch)
        autogenlines.append('epicsEnvSet(\"IOC\", %s)\n' % iocdir)
        autogenlines.append('epicsEnvSet("ENGINEER", "ASKAP Operations")\n')
        autogenlines.append('epicsEnvSet("LOCATION", "MRO")\n')
        autogenlines.append('cd ${IOC_ROOT}/iocBoot/${IOC}\n')
        if not os.path.exists(iocbootdir):
            os.makedirs(iocbootdir)
        # List files inside ioctemplatedir (where st.cmd.template and
        # other files are located)
        iocfilelist = os.listdir(ioctemplatedir)
        for iocfile in iocfilelist:
            if os.path.isdir(os.path.join(ioctemplatedir, iocfile)):
                continue  # Skip directories
            if iocfile == 'st.cmd.template':
                # Open the startup script and add the auto-generated lines
                stcmdfile = os.path.join(iocbootdir, 'st.cmd')
                with open(stcmdfile, 'w') as stcmdfileobj:
                    stcmdfileobj.writelines(autogenlines)
                    # Open template file and write all its contents to stcmd
                    with open(os.path.join(ioctemplatedir,
                                           'st.cmd.template'), 'rU') \
                            as templfileobj:
                        stcmdfileobj.writelines(templfileobj.readlines())
            else:
                # Copy the file in installdir/iocBoot
                shutil.copy(os.path.join(ioctemplatedir, iocfile), iocbootdir)
            # Add executable permissions to st.cmd
            if os.path.exists(os.path.join(iocbootdir, 'st.cmd')):
                utils.run('chmod a+x %s' % os.path.join(iocbootdir, 'st.cmd'))

    def _copy_procserv(self):
        pass
