#!/usr/bin/env python2

import os

paths = os.listdir("/srv/http/projects")
os.chdir("/srv/http/projects")

for name in paths:
  os.chdir(name)
  os.system("git pull")
  os.chdir("/srv/http/projects")
