#!/usr/bin/env python3

from json import load
from glob import glob
from os import path
from socket import gethostname
from re import sub,match,search

import sys,getopt

def on_error():
    print("This generates new depedencies files:")
    print("make_depends.py -s <system>")
    print("system is typically default")

try:
    opts,args = getopt.getopt(sys.argv[1:],"hs:",["help","system="])
except:
    on_error()
    sys.exit(2)
for opt,arg in opts:
    if opt in ("-h","--help"):
      on_error()
      sys.exit()
    elif opt in ("-s", "--system"):
      system = arg;



yanda_packages = load(open("yanda_packages.json"))
files=glob("**/dependencies." + system,recursive=True)
for eachfile in files:
    print(eachfile)
    with open(eachfile,"r") as original:
        h = "." + gethostname()
        root = path.splitext(eachfile)[0]
        p = root + h
        lines = original.readlines()
        with open(p,"w") as newfile:
          for line in lines:
            matched=False
            for eachpackage in yanda_packages.keys():
              if match("^" + eachpackage+"=",line):
                if search(";",line):
                  newfile.write(sub(r'=\w+;', "=" + yanda_packages[eachpackage] + ";", line))
                else:
                  newfile.write(sub(r'=\S+',"=" + yanda_packages[eachpackage] ,line))
                matched=True
            if (matched == False):
              newfile.write(line)

# print(sub(r'/usr/local', yanda_packages[eachpackage], line))


