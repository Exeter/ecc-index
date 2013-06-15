#!/usr/bin/env python
import sqlite3
import auth
import os
import urlparse
import simplejson as json

if __name__ == "__main__":
  #Parse the path and such
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]
  
  #Connect to the users database
  conn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  
  print "Content-Type: application/json"
  print ""

  #Create the user and tell the client whether or not this was successfull
  print json.dumps({
    "success": auth.createUser(conn, qwargs["uname"], qwargs["verifier"], qwargs["salt"])
  })
  
  conn.commit()
  conn.close()
