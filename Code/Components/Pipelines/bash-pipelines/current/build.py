import os, shutil
from askapdev.rbuild.builders import Data as Builder
#from askapdev.rbuild.builders import Builder
#import askapdev.rbuild.utils as utils

builder = Builder("scripts")
builder.build()
#utils.copy_tree('scripts','install/bin')
