import shutil
import os
import sys
import urllib2
import ConfigParser

version = "1.11.4"
#version = "12.0"
#version = "14.0.6"
#version = "14.0.6"
virtualenv = "virtualenv-%s" % version
python_ver = sys.version[:3]
python_exe = sys.executable
askap_root = os.environ.get("ASKAP_ROOT", os.path.abspath("../../"))
install_dir = "%s/install" % os.getcwd()

cfg_parser = ConfigParser.ConfigParser()
config_file = "%s/bootstrap.ini" % askap_root
cfg_parser.read(config_file)


if os.path.isdir(install_dir):
    shutil.rmtree(install_dir)

if not os.path.exists(virtualenv):
    os.mkdir(virtualenv)
os.chdir(virtualenv)
remote = os.environ.get("RBUILD_REMOTE_ARCHIVE", "")
pkg = "%s.tar.gz" % virtualenv
if not os.path.exists(pkg):
    if remote:
        pth = os.path.join(remote, pkg)
        url = urllib2.urlopen(pth)
        with open(pkg, "wb") as of:
            of.write(url.read())
        print "Downloading", remote

os.system("tar xvzf %s" % pkg)
os.chdir("%s" % virtualenv)
os.system("%s virtualenv.py --no-site-packages %s" % (python_exe, askap_root))
# For use with release process.
os.system("%s virtualenv.py --no-site-packages %s" % (python_exe, install_dir))

for name, version in cfg_parser.items("Python Dependencies"):
        install_line=". %s/bin/activate && pip install %s" % (askap_root,version)
        
        print "running ", install_line
        os.system(install_line)
