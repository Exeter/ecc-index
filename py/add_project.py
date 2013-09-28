#!/usr/bin/env python
import sys
import simplejson as json

if __name__ == "__main__":
  # Read current projects
  projects_file = open("/srv/http/server_projects.json", "r")
  projects = json.load(projects_file)
  projects_file.close()

  # Add the new one
  projects["projects"].append({
    "name": sys.argv[1],
    "url": sys.argv[2]
  })

  # Write it
  projects_file = open("/srv/http/server_projects.json", "w")
  json.dump(projects, projects_file)
  projects_file.close()
