#!/usr/bin/env python
import sqlite3
import simplejson as json
import os
import urlparse
import time

if __name__ == "__main__":
  #Parse the url we just got
  path = os.environ["PATH_INFO"].split("/")
  qwargs = urlparse.parse_qs(os.environ["QUERY_STRING"])
  
  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  conn = sqlite3.connect("/home/anthony/ecc-index/db/news.db")
  c = conn.cursor()
  virtual_last = int(float(qwargs["last"]) if "last" in qwargs else time.time())

  #Make sure that this table exists
  c.execute("""
    CREATE TABLE IF NOT EXISTS news (id INTEGER PRIMARY KEY ASC, title TEXT, body TEXT, timestamp INTEGER)
  """)

  c.execute("""
    SELECT * FROM news WHERE timestamp<? ORDER BY timestamp DESC LIMIT 10
  """, (virtual_last,)) #If there is no "last" argument, default to latest
  
  results = c.fetchall()
  posts = {
    "posts": [],
    "last": virtual_last
  }
  for row in results:
    posts["posts"].append({
      "title": row[1],
      "body": row[2],
      "timestamp": row[3]
    })
    if row[3] < posts["last"]:
      posts["last"] = row[3]
  print "Content-Type:application/json"
  print ""
  print json.dumps(posts)
