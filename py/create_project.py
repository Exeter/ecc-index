#!/usr/bin/env python2
import sqlite3
import simplejson as json
import os

if __name__=="__main__":
  conn = sqlite3.connect("/home/daemon/ecc-index/db/projects.db")
  uconn = sqlite3.connect("/home/daemon/ecc-index/db/users.db")
  c = conn.cursor()
  u = uconn.cursor()
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, manifest TEXT, analytics TEXT, updated TEXT)
  """)
  u.execute("""
    CREATE TABLE IF NOT EXISTS team (id INTEGER PRIMARY KEY ASC, uname TEXT, projects TEXT)
  """)
  
  name = raw_input("Project Name: ")
  team = raw_input("Team Members (space-delimited): ").split(" ")
  github = "https://api.github.com/repos/Exeter/" + name
  
  for member in team:
    u.execute("""
      SELECT * FROM team WHERE uname=?
    """, (member,))
    row = u.fetchone()
  
    if row is not None:
      current_team = json.loads(row[2])
      current_team.append(name)
      u.execute("""
        UPDATE team SET projects=? WHERE id=?
      """, (row[0], json.dumps(current_team)))
    else:
      u.execute("""
        INSERT INTO team (uname, projects) VALUES (?, ?)
      """, (member, json.dumps([name])))
  
  c.execute("""
    INSERT INTO projects (name, team, github, manifest, analytics) VALUES (?, ?, ?, ?, ?)
  """, (name, json.dumps(team), github, os.path.realpath("/home/daemon/projects/" + name), json.dumps({"endpoints":{}, "urls":{}, "failures":[]})))
  
  conn.commit()
  uconn.commit()

  conn.close()
  uconn.close()
