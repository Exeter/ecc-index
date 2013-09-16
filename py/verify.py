#!/usr/bin/env python2
import os
import sys
import urlparse
import base64
import auth
import simplejson as json

if __name__ == "__main__":
  #Parse the path and such
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])

  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]
  
  #Connect to the users database
  conn = auth.initDB("/home/daemon/ecc-index/db/users.db")
  
  #Create the session key table if it's not there yet, and delete any old session keys
  c = conn.cursor()
  c.execute("""
    SELECT * FROM keys WHERE uname=?
  """, (qwargs["uname"],))
  
  key = base64.b64decode(c.fetchone()[2])
  
  print "Content-Type: application/json"
  print ""

  decrypted = auth.decrypt(key, qwargs["message"])

  if decrypted == "SRP_CLIENT_SUCCESS_MESSAGE":
    print json.dumps({
      "message": auth.encrypt(key, "SRP_SERVER_SUCCESS_MESSAGE")
    })
  else:
    print json.dumps({
      "error":True,
      "decrypted":decrypted
    })
