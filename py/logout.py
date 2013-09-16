#!/usr/bin/env python2
import sqlite3
import auth
import urlparse
import os
import base64
import simplejson as json

if __name__ == "__main__":
  print "Content-type: application/json"
  print ""

  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  for key in qwargs:
    qwargs[key] = qwargs[key][0]
  
  conn = auth.initDB("/home/daemon/ecc-index/db/users.db")
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
    sig = auth.decrypt(key, qwargs["signature"])
    if sig == "SRP_LOGOUT_COMMAND":
      c.execute("""
        DELETE FROM keys WHERE uname=?
      """, (qwargs["uname"],))
      conn.commit()
      print json.dumps({
        "success": True
      })
    else:
      print json.dumps({
        "success": False,
        "error": "DATA CORRUPTION"
      })
  else:
    print json.dumps({
      "success": False,
      "error": "NO ESTABLISHED SESSKEY"
    })
