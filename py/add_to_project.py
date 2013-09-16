#!/usr/bin/env python2
import sqlite3
import sys
import simplejson as json
import os
import urlparse
import auth
import base64

if __name__ == "__main__":
  # Parse path and query string
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  # Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  pconn = sqlite3.connect("/home/daemon/ecc-index/db/projects.db")
  uconn = auth.initDB("/home/daemon/ecc-index/db/users.db")
  c = pconn.cursor()
  u = uconn.cursor()
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, manifest TEXT, analytics TEXT, updated TEXT)
  """)
  u.execute("""
    CREATE TABLE IF NOT EXISTS team (id INTEGER PRIMARY KEY ASC, uname TEXT, projects TEXT)
  """)
  u.execute("""
    CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY ASC, uname TEXT, key TEXT);
  """)

  u.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))

  keyrow = u.fetchone()

  result = {
    "success": True
  }

  if keyrow is not None:
    request_data = json.loads(auth.decrypt(base64.b64decode(keyrow[2]), qwargs["info"]))
    
    if request_data is not None:
      name = request_data["target"]
      project = request_data["project"]
      

      # Add this project to this user's list of projects
      u.execute("""
        SELECT * FROM team WHERE uname=?
      """, (name,))
      row = u.fetchone()

      if row is not None:
        current_projects = json.loads(row[2])
        if project not in current_projects:
          current_projects.append(project)
          u.execute("""
            UPDATE team SET projects=? WHERE id=?
          """, (json.dumps(current_projects), row[0]))
      else:
        u.execute("""
          SELECT * FROM users WHERE uname=?
        """, (name,))

        if u.fetchone() is not None:
          u.execute("""
            INSERT INTO team (uname, projects) VALUES (?, ?)
          """, (name, json.dumps([project])))
        else:
          result["success"] = False
          result["error"] = "NO SUCH TARGET"
      
      if result["success"]:
        # Add this user to this project's list of users
        c.execute("""
          SELECT * FROM projects WHERE name=?
        """, (project,))
        row = c.fetchone()

        if row is not None:
          current_team = json.loads(row[2])
          if name not in current_team:
            current_team.append(name)
            c.execute("""
              UPDATE projects SET team=? WHERE id=?
            """, (json.dumps(current_team), row[0]))
        else:
          result["success"] = False
          result["error"] = "NO SUCH PROJECT"

        uconn.commit()
        pconn.commit()
    else:
      result["success"] = False
      result["error"] = "AUTHENTICATION FAILURE"
  else:
    result["success"] = False
    result["error"] = "NO SUCH USER"
print "Content-type: application/json"
print ""
print json.dumps(result)
