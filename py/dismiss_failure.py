#!/usr/bin/env python
import httplib
import urlparse
import os
import auth
import sqlite3
import base64
import simplejson as json

if __name__ == "__main__":
  # Declare our content-type
  print "Content-Type: application/json"
  print ""

  # Parse the path and query string arguments
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  # Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  # Connect to the projects database
  conn = sqlite3.connect("/home/anthony/ecc-index/db/projects.db")
  c = conn.cursor()

  # Make sure that the projects table exists
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, path TEXT, analytics TEXT, updated TEXT)
  """)
  
  # Get this user's session key
  kconn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  k = kconn.cursor()
  k.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))
  keyrow = k.fetchone()
  if keyrow is not None:
    key = base64.b64decode(keyrow[2])

    # Get the row that we want
    c.execute("""
      SELECT * FROM projects WHERE name=?
    """, (path[2],))
     
    row = c.fetchone()
    
    if row is not None:
      analytics = json.loads(row[5])
      analytics["failures"] = filter(lambda (x): x["id"] != rid, analytics["failures"])
      print auth.encrypt(key, json.dumps({
        "success": True
      })
    else:
      print auth.encrypt(key, json.dumps({
        "error": "NO SUCH PROJECT"
      })
