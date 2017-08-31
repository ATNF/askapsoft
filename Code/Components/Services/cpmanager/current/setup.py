from askapdev.rbuild.setup import setup
from askapdev.rbuild.dependencies import Dependency
from setuptools import find_packages

dep = Dependency()
dep.add_package()

ROOTPKG   = 'askap'
COMPONENT = 'ingest'

setup(name = '%s.%s' % (ROOTPKG, COMPONENT),
      version = 'current',
      description = 'ASKAP CP Manager module',
      author = 'Xinyu Wu',
      author_email = 'xinyu.wu@csiro.au',
      url = 'http://svn.atnf.csiro.au/askap',
      keywords = ['ASKAP', 'CP Manager'],
      long_description = '''@@@long_description@@@''',
      packages = find_packages(),
      namespace_packages = [ROOTPKG,],
      license = 'GPL',
      zip_safe = 0,
      dependency = dep,
      scripts = ["scripts/ingestservice.py"],

      # Uncomment if using unit tests
     test_suite = "nose.collector", install_requires=['nose']
      )
