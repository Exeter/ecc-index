import os

paths = os.listdir("/srv/http/projects")
os.chdir("/srv/http/projects")

for name in path:
  os.chdir(name)
  os.system("git pull")
  os.chdir("/srv/http/projects")
