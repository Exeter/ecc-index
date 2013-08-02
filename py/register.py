#!/usr/bin/env python
import sqlite3
import auth
import os
import urlparse
import base64
import simplejson as json

if __name__ == "__main__":
  #Declare our content-type
  print "Content-Type: application/json"
  print ""

  #Parse the path and such
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  #Connect to the users database
  conn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  c = conn.cursor()

  c.execute("""
    CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY ASC, uname TEXT, key TEXT)
  """)

  c.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))

  keyrow = c.fetchone()

  if keyrow is not None:
    key = base64.b64decode(keyrow[2])
    info = json.loads(auth.decrypt(key, qwargs["info"]))
    
    if info is not None:
      #Create the user and tell the client whether or not this was successfull
      print auth.encrypt(key, json.dumps({
        "success": auth.createUser(conn, info["uname"], info["verifier"], info["salt"])
      }))
      
      conn.commit()
      conn.close()
    else:
      print auth.encrypt(key, json.dumps({
        "error": "DATA CORRUPTION"
      }))
  else:
    print json.dumps({
      "error": "NO ESTABLISHED SESSKEY"
    })
