import os, shutil
from askapdev.rbuild.builders import Data as DataBuilder
from askapdev.rbuild.builders import Builder
import askapdev.rbuild.utils as utils

builder = Builder(".")
builder.build()
databuilder = DataBuilder("scripts")
databuilder.build()

if os.access(builder._initscript_name,os.F_OK):
    bindir=os.getcwd()+'/install/bin'
    initfile=open(builder._initscript_name,'a+')
    initfile.write("""
#The following is required by the pipeline scripts.
PIPELINEDIR=%s
export PIPELINEDIR
"""%bindir)

    version=utils.get_svn_revision()
    initfile.write("""
#This is the release version
PIPELINE_VERSION=%s
export PIPELINE_VERSION
"""%version)
    
    initfile.close()

