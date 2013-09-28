#!/usr/bin/env python2
import httplib
import simplejson

request = httplib.HTTPSConnection("api.github.com")
request.putrequest("GET", "/users/Exeter/repos")
request.putheader("User-Agent", "dabbler0")
request.endheaders()
loaded = simplejson.loads(request.getresponse().read())

project_list = []
projects_file = open("/srv/http/github_projects.json", "w")
for repository in loaded:
  project_list.append({"name":repository["name"], "url":repository["html_url"]})
projects_file.write(simplejson.dumps({"projects":project_list}))
projects_file.close()
