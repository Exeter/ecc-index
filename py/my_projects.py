#!/usr/bin/env python
import urlparse
import base64
import auth
import os
import simplejson as json

if __name__ == "__main__":
  # Parse path and query string
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])
  
  # Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  uconn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  u = uconn.cursor()

  u.execute("""
    CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY ASC, uname TEXT, key TEXT);
  """)
  u.execute("""
    CREATE TABLE IF NOT EXISTS team (id INTEGER PRIMARY KEY ASC, uname TEXT, projects TEXT)
  """)

  u.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))

  keyrow = u.fetchone()

  print "Content-Type: application/json"
  print ""
  
  if keyrow is None:
    print json.dumps({
      "error": "NO ESTABLISHED SESSKEY"
    })
  else:
    key = base64.b64decode(keyrow[2])
    
    u.execute("""
      SELECT * FROM team WHERE uname=?
    """, (qwargs["uname"],))

    teamrow = u.fetchone()
    if teamrow is None:
      print auth.encrypt(key, json.dumps({
        "teams":[]
      }))
    else:
      print auth.encrypt(key, json.dumps({
        "teams":json.loads(teamrow[2])
      }))
