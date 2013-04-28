import os

paths = os.listdir("/srv/http/cgi-bin")
os.chdir("/srv/http/cgi-bin")

for name in path:
  os.chdir(name)
  os.system("git pull")
  os.chdir("/srv/http/cgi-bin")
