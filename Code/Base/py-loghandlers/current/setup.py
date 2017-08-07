from askapdev.rbuild.setup import setup
from askapdev.rbuild.dependencies import Dependency
from setuptools import find_packages

dep = Dependency()
dep.add_package()

ROOTPKG = "askap"
PKGNAME = "loghandlers"

setup(name = "%s.%s" % (ROOTPKG, PKGNAME),
      version = 'current',
      description = 'ASKAP logging handler extensions',
      author = 'Malte Marquarding',
      author_email = 'Malte.Marquarding@csiro.au',
      url = 'http://svn.atnf.csiro.au/askap',
      keywords = ['ASKAP', 'logging', 'Base', 'Ice'],
      long_description = '''Adddional logging handler for python. This includes and ice handler.''',
      packages = find_packages(),
      namespace_packages = [ROOTPKG],
      license = 'GPL',
      zip_safe = 0,
      dependency = dep,
      scripts = ["scripts/askap_logsubscriber.py"],
      package_data = {"": ["config/*.ice",
                           "config/*.ice_cfg"] },
      test_suite = "nose.collector",
)
