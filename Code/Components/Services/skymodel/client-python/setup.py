from askapdev.rbuild.setup import setup
from askapdev.rbuild.dependencies import Dependency
from setuptools import find_packages

dep = Dependency()
dep.add_package()

ROOTPKG   = 'askap'
COMPONENT = 'smsclient'

setup(
    name = '%s.%s' % (ROOTPKG, COMPONENT),
    version = 'current',
    description = 'ASKAP Skymodel Service Client',
    author = 'Daniel Collins',
    author_email = 'daniel.collins@csiro.au',
    url = 'http://svn.atnf.csiro.au/askap',
    keywords = ['ASKAP', 'Skymodel', 'client', 'SMS'],
    long_description = '''@@@long_description@@@''',
    packages = find_packages(),
    namespace_packages = [ROOTPKG,],
    license = 'GPL',
    zip_safe = True,
    dependency = dep,
    test_suite = "nose.collector",
    install_requires = [
        'setuptools',
    ],
)
