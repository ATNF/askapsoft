# @file
# SConstruct build script for this module.
# Package dependencies are read from 'dependencies.default'
#
# @author Malte Marquarding <Malte.Marquarding@csiro.au>
#

# Always import this
from askapenv import env
import sys

env.AppendUnique(CCFLAGS=['-O3'])

# On Mac use the inbuilt Accelerate framework so no longer get the linker flag from
# dependency analysis so need to make it explict here.
if sys.platform == 'darwin':
    env.AppendUnique(LINKFLAGS=['-llapack'])

# create build object with library name
pkg = env.AskapPackage("scimath")

# add sub packages 
pkg.AddSubPackage("fitting")
pkg.AddSubPackage("utils")
pkg.AddSubPackage("fft")

# run the build process
pkg()
