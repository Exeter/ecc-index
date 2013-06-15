#!/usr/bin/env python
import httplib
import simplejson

request = httplib.HTTPSConnection("api.github.com")
request.putrequest("GET", "/users/Exeter/repos")
request.putheader("User-Agent", "dabbler0")
request.endheaders()
loaded = simplejson.loads(request.getresponse().read())
print loaded

project_list = []
for repository in loaded:
  project_list.append({"name":repository["name"], "url":repository["html_url"]})
  open("/srv/http/projects.json", "w").write(simplejson.dumps({"projects":project_list}))
