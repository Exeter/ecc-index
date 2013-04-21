#!/usr/bin/env python
import http.client
import simplejson

request = http.client.HTTPSConnection("api.github.com");
request.request("GET","/users/Exeter/repos?client_id=a951833eb1496c8c32ef&client_secret=f338d8a20721decdae676e58c69a127aafdadafc");
loaded = simplejson.loads(request.getresponse().read());

project_list = [];
for repository in loaded:
  project_list.append({"name":repository["name"], "url":repository["html_url"]})
open("/srv/http/projects.json", "w").write(simplejson.dumps({"projects":project_list}));
