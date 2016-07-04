import os
from askapdev.rbuild.setup import setup
from askapdev.rbuild.dependencies import Dependency

dep = Dependency(silent=False)
dep.DEPFILE = "../dependencies"
dep.add_package()

setup(name = "testpkg",
      dependency = dep,
      test_suite = "nose.collector",
)
