#!/usr/bin/env python
import sqlite3
import os
import re
import sys
import simplejson as json

def parseManifest(manifest, url):
  manifest_file = open(manifest, "r")
  for line in manifest_file:
    reg_mark = line.index(" ")
    com_mark = line.rindex(" ")
    match = re.match("^%s$" % line[:reg_mark], url)
    if match:
      com = line[com_mark + 1:]
      if com == "MANIFEST":
        new_manifest_name = match.expand(line[reg_mark + 1:com_mark])
        result = parseManifest(new_manifest_name, url)
        manifest_file.close()
        return result[0], (new_manifest_name, result[1])
      else:
        manifest_file.close()
        return (match.expand(line[reg_mark + 1:com_mark]), com), (manifest, ())

if __name__ == "__main__":
  loaded = {}
  
  # Open our database connections
  conn = sqlite3.connect("/home/anthony/ecc-index/db/projects.db")
  c = conn.cursor()

  # Ensure that the wanted table exists
  c.execute("""
    CREATE TABLE IF NOT EXISTS projects (id INTEGER PRIMARY KEY ASC, name TEXT, team TEXT, github TEXT, path TEXT, analytics TEXT, updated TEXT)
  """)
  
  """
    Parse the request log file
  """

  # Open the log file
  url_dump = open("/srv/http/logs/access_log", "r")
  
  for line in url_dump:
    #sys.stdout.write(line)
    request = line[line.index("\"") + 1:line.rindex("\"")].split(" ")
    index = request[1].find("?")
    if index > 0:
      request[1] = request[1][0:index]
    if request[1] == "" or request[1] == "/":
      request[1] = "/index.html"
    parsed = parseManifest("/srv/http/manifest.txt", request[1])

    if parsed is None:
      continue
    
    project_to_modify = None
    remaining_projects = parsed[1]
    while len(remaining_projects) > 0:
      endpoint = project_to_modify if project_to_modify is not None else parsed[0][0]
      project_to_modify = remaining_projects[0]
      remaining_projects = remaining_projects[1]
      realpath = os.path.dirname(os.path.realpath(project_to_modify))
    
      # If we've loaded this already, increment it
      if realpath in loaded:
        if loaded[realpath] is not None:
          # Log the endpoint
          if endpoint in loaded[realpath]["endpoints"]:
            loaded[realpath]["endpoints"][endpoint] += 1
          else:
            loaded[realpath]["endpoints"][endpoint] = 1
          # Log the url
          if request[1] in loaded[realpath]["urls"]:
            loaded[realpath]["urls"][request[1]] += 1
          else:
            loaded[realpath]["urls"][request[1]] = 1

      # Otherwise, load it from the database
      else:
        c.execute("""
          SELECT * FROM projects WHERE path=?
        """, (realpath,))
        row = c.fetchone()
        if row is not None:
          info = json.loads(row[5])
          loaded[realpath] = info
          # Log the endpoint
          if endpoint in loaded[realpath]["endpoints"]:
            loaded[realpath]["endpoints"][endpoint] += 1
          else:
            loaded[realpath]["endpoints"][endpoint] = 1
          # Log the url
          if request[1] in loaded[realpath]:
            loaded[realpath]["urls"][request[1]] += 1
          else:
            loaded[realpath]["urls"][request[1]] = 1
        else:
          loaded[realpath] = None

  """
    Parse the failure logs
  """
  
  failure = open("/srv/http/logs/failure_log", "r")

  while True:
    faildict = {}

    # Get the request line
    request = failure.readline()
    if len(request) == 0:
      break
    else:
      print request.rstrip()
      request = request.rstrip().split(" ")
      assert ((request[0] == "FAIL" or request[0] == "ARRESTED") and len(request) == 5), "Misformatted log file."
    
    parsed = parseManifest("/srv/http/manifest.txt", request[2])

    faildict["request"] = request
    
    assert (os.path.samefile(parsed[0][0], request[4])), "Outdated manifest file."

    # Get the time line
    time = failure.readline().rstrip().split(" ")
    assert (time[0] == "TIME" and len(time) == 2), "Misformatted log file."
    faildict["timestamp"] = int(time[1])
    

    # The following only applies if the child exited normally:
    if request[0] == "FAIL":
      # Get the error code line
      error_code = failure.readline().rstrip().split(" ")
      assert (error_code[0] == "EXIT" and error_code[1] == "CODE" and len(error_code) == 3), "Misformatted log file."
      faildict["error_code"] = int(error_code[2])

    # Get the stdin
    assert (failure.readline().rstrip() == "IN:"), "Misformatted log file."
    input_line = failure.readline()
    total_input = ""
    while input_line[0:2] == "  ":
      total_input += input_line[2:]
      input_line = failure.readline()

    faildict["stdin"] = total_input
    
    # The following only applies in the child existed normally:
    if request[0] == "FAIL":
      assert (input_line.rstrip() == "OUT:"), "Misformatted log file."

      # Get the stdout
      output_line = failure.readline()
      total_stdout = ""
      while output_line[0:2] == "  ":
        total_stdout += output_line[2:]
        output_line = failure.readline()
      
      faildict["stdout"] = total_stdout

      assert (output_line.rstrip() == "ERR:"), "Misformatted log file."
      
      # Get the stderr
      error_line = failure.readline()
      total_stderr = ""
      while error_line[0:2] == "  ":
        total_stderr += error_line[2:]
        error_line = failure.readline()
      
      faildict["stderr"] = total_stderr

    print faildict

    project_to_modify = None
    remaining_projects = parsed[1]
    
    while len(remaining_projects) > 0:
      endpoint = project_to_modify if project_to_modify is not None else parsed[0][0]
      project_to_modify = remaining_projects[0]
      remaining_projects = remaining_projects[1]
      realpath = os.path.dirname(os.path.realpath(project_to_modify))

      print project_to_modify
    
      # If we've loaded this already, increment it
      if realpath in loaded:
        if loaded[realpath] is not None:
          loaded[realpath]["failures"].append(faildict)
      # Otherwise, load it from the database
      else:
        c.execute("""
          SELECT * FROM projects WHERE path=?
        """, (realpath,))
        row = c.fetchone()
        if row is not None:
          info = json.loads(row[5])
          loaded[realpath] = info
          loaded[realpath]["failures"].append(faildict)
        else:
          loaded[realpath] = None

  # Save it all in our database
  for key in loaded:
    if loaded[key] is not None:
     c.execute("""
        UPDATE projects SET analytics=? WHERE path=?
      """, (json.dumps(loaded[key]), key))
  conn.commit()
  conn.close()
