#!/usr/bin/env python
import sqlite3
import os
import re
import sys
import simplejson as json

def parseManifest(manifest, url):
  manifest_file = open(manifest, "r")
  for line in manifest_file:
    reg_mark = line.index(" ")
    com_mark = line.rindex(" ")
    match = re.match("^%s$" % line[:reg_mark], url)
    if match:
      com = line[com_mark + 1:]
      if com == "MANIFEST":
        new_manifest_name = match.expand(line[reg_mark + 1:com_mark])
        result = parseManifest(new_manifest_name, url)
        manifest_file.close()
        return result[0], (new_manifest_name, result[1])
      else:
        manifest_file.close()
        return (match.expand(line[reg_mark + 1:com_mark]), com), (manifest, ())

if __name__ == "__main__":
  loaded = {}
  
  # Open our database connections
  conn = sqlite3.connect("/home/anthony/ecc-index/db/projects.db")
  c = conn.cursor()

  # Ensure that the wanted table exists
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, manifest TEXT, analytics TEXT, updated TEXT)
  """)

  # Open the dump file and the manifest file
  url_dump = open("/srv/http/logs/url.dump", "r")
  
  for line in url_dump:
    sys.stdout.write(line)
    index = line.index(" ")
    parsed = parseManifest("/srv/http/manifest.txt", line[:index])
    
    project_to_modify = None
    remaining_projects = parsed[1]
    while len(remaining_projects) > 0:
      endpoint = project_to_modify if project_to_modify is not None else parsed[0][0]
      project_to_modify = remaining_projects[0]
      remaining_projects = remaining_projects[1]
      realpath = os.path.realpath(project_to_modify)
    
      # If we've loaded this already, increment it
      if realpath in loaded:
        if loaded[realpath] is not None:
          if parsed[0][0] in loaded[realpath]:
            loaded[realpath]["endpoints"][endpoint] += int(line[index + 1:])
          else:
            loaded[realpath]["endpoints"][endpoint] = int(line[index + 1:])
      
      # Otherwise, load it from the database
      else:
        c.execute("""
          SELECT * FROM projects WHERE manifest=?
        """, (os.path.realpath(project_to_modify),))
        row = c.fetchone()
        if row is not None:
          info = json.loads(row[5])
          loaded[realpath] = info
          if parsed[0][0] in loaded[realpath]:
            loaded[realpath]["endpoints"][endpoint] += int(line[index + 1:])
          else:
            loaded[realpath]["endpoints"][endpoint] = int(line[index + 1:])
        else:
          loaded[realpath] = None
  for key in loaded:
    c.execute("""
      UPDATE projects SET analytics=? WHERE manifest=?
    """, (json.dumps(loaded[key]), key))
  conn.commit()
  conn.close()
