#!/usr/bin/env python
import sqlite3
import simplejson as json
import os
import urlparse
import time

if __name__ == "__main__":
  #Parse the url we just got
  urlpath = urlparse.urlparse(os.environ["PATH_INFO"])
  path = urlpath.path.split("/")
  qwargs = urlparse.parse_qs(urlpath.query)
  
  #Enforce one value per query string argument
  for key in qwargs:
    qwargs[key] = qwargs[key][0]

  conn = sqlite3.connect("/home/anthony/Projects/new_ecc/news.db")
  c = conn.cursor()
  
  #Make sure that this table exists
  c.execute("""
    CREATE TABLE IF NOT EXISTS news (id INTEGER PRIMARY KEY ASC, title TEXT, body TEXT, timestamp INTEGER)
  """)

  c.execute("""
    SELECT * FROM news WHERE timestamp<? ORDER BY timestamp DESC LIMIT 10
  """, (int(qwargs["last"] if "last" in qwargs else time.time()),)) #If there is no "last" argument, default to lastest

  results = c.fetchall()
  posts = {
    "posts": [],
    "last": time.time()
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
