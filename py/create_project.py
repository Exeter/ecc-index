#!/usr/bin/env python
import sqlite3
import simplejson as json
import os

if __name__=="__main__":
  conn = sqlite3.connect("/home/anthony/ecc-index/db/projects.db")
  c = conn.cursor()
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, manifest TEXT, analytics TEXT, updated TEXT)
  """)

  name = raw_input("Project Name: ")
  team = raw_input("Team Members (space-delimited): ").split(" ")
  manifest = raw_input("Manifest Location: ")
  github = "https://api.github.com/repos/Exeter/" + name

  c.execute("""
    INSERT INTO projects (name, team, github, manifest, analytics) VALUES (?, ?, ?, ?, ?)
  """, (name, json.dumps(team), github, os.path.realpath(manifest), json.dumps({"endpoints":{}})))

  conn.commit()
