#!/usr/bin/env python

import os

paths = os.listdir("/srv/http/cgi-bin")

print (paths)

os.chdir("/srv/http/cgi-bin")

for name in paths:
  os.chdir(name)
  os.system("git pull")
  os.chdir("/srv/http/cgi-bin")
