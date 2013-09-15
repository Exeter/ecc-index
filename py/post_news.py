#!/usr/bin/env python
import sqlite3
import simplejson as json
import os
import urlparse
import time
import auth
import base64
import sys

if __name__ == "__main__":
  #Parse the url we just got
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  #Read stdin. We don't care about request headers.
  line = sys.stdin.readline()
  while line != "\n":
    line = sys.stdin.readline()
  
  stdinstr = sys.stdin.read()

  pargs = urlparse.parse_qs(stdinstr)

  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  for key in pargs:
    pargs[key] = pargs[key][0]

  nconn = sqlite3.connect("/home/anthony/ecc-index/db/news.db")
  n = nconn.cursor()

  #Make sure that this table exists
  n.execute("""
    CREATE TABLE IF NOT EXISTS news (id INTEGER PRIMARY KEY ASC, title TEXT, body TEXT, timestamp INTEGER)
  """)

  #Connect to the users database
  uconn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  
  #Create the session key table if it's not there yet, and delete any old session keys
  u = uconn.cursor()
  u.execute("""
    CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY ASC, uname TEXT, key TEXT);
  """)
  u.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))
  
  keyrow = u.fetchone()

  if keyrow is not None:
    key = base64.b64decode(keyrow[2])
    info = json.loads(auth.decrypt(key, pargs["info"]))
    
    n.execute("""
      INSERT INTO news (title, body, timestamp) VALUES (?, ?, ?)
    """, (info["title"], info["body"], time.time()))
  
    nconn.commit()

    print "Content-type: application/json"
    print ""
    print auth.encrypt(key, json.dumps({
      "success": True
    }))
  else:
    print "Content-type: application/json"
    print ""
    print json.dumps({
      "error": "NO ESTABLISHED SESSKEY"
    })
