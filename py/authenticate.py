#!/usr/bin/env python
import os
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
  conn = auth.initDB("/home/anthony/ecc-index/db/users.db")
  
  #Create the session key table if it's not there yet, and delete any old session keys
  c = conn.cursor()
  c.execute("""
    CREATE TABLE IF NOT EXISTS keys (id INTEGER PRIMARY KEY ASC, uname TEXT, key TEXT);
  """)
  c.execute("""
    DELETE FROM keys WHERE uname=?
  """, (qwargs["uname"],))

  #Generate the session key from what we're given
  kdict = auth.generateKey(conn, qwargs["uname"], int(qwargs["A"], 16))

  if kdict is None:
    print "Content-Type: application/json"
    print ""
    print json.dumps({
      "error": "NO SUCH USER"
    })

  #Save it in our keys database
  c.execute("""
    INSERT INTO keys (uname, key) VALUES (?, ?)
  """, (qwargs["uname"], base64.b64encode(kdict["K"])))
  
  conn.commit()
  conn.close()
  
  #Output
  print "Content-Type: application/json"
  print ""

  print json.dumps({
    "s": kdict["s"],
    "B": kdict["B"]
  })
