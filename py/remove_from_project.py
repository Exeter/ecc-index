#!/usr/bin/env python2
import sqlite3
import sys
import simplejson as json

if __name__ == "__main__":
  pconn = sqlite3.connect("/home/daemon/ecc-index/db/projects.db")
  uconn = sqlite3.connect("/home/daemon/ecc-index/db/users.db")
  c = pconn.cursor()
  u = uconn.cursor()
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, manifest TEXT, analytics TEXT, updated TEXT)
  """)
  u.execute("""
    CREATE TABLE IF NOT EXISTS team (id INTEGER PRIMARY KEY ASC, uname TEXT, projects TEXT)
  """)
  
  name = raw_input("Username: ")
  project = raw_input("Project name: ")
  
  # Add this project to this user's list of projects
  u.execute("""
    SELECT * FROM team WHERE uname=?
  """, (name,))
  row = u.fetchone()

  if row is not None:
    current_projects = json.loads(row[2])
    if project in current_projects:
      current_projects = filter(lambda x: x != project, current_projects)
      u.execute("""
        UPDATE team SET projects=? WHERE id=?
      """, (json.dumps(current_projects), row[0]))
    else:
      print "(users.db) Already off that team."
  else:
    print "(users.db) Already off that team."
  # Add this user to this project's list of users
  c.execute("""
    SELECT * FROM projects WHERE name=?
  """, (project,))
  row = c.fetchone()

  if row is not None:
    current_team = json.loads(row[2])
    if name in current_team:
      current_team = filter(lambda x: x != name, current_team)
      c.execute("""
        UPDATE projects SET team=? WHERE id=?
      """, (json.dumps(current_team), row[0]))
    else:
      print "(projects.db) Already off that team."
  else:
    print "(projects.db) Already off that team."
  uconn.commit()
  pconn.commit()
