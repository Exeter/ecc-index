#!/usr/bin/env python
import httplib
import simplejson
import datetime
import dateutil.parser
import sqlite3
import time
from templet import stringfunction

@stringfunction
def format_commit(commit):
  """
    <div class="ag_commit">
      <div class="ag_commit_message">
        <a href=${commit["link"]}>${commit["message"]}</a>
      </div>
      <div class="ag_commit_meta">
        <a class="ag_commit_committer" href="${commit["user_link"]}">${commit["committer"]}</a>
        <span class="ag_commit_timestamp">${commit["timestamp"]}</span>
      </div>
    </div>
  """

@stringfunction
def format_repo(repo):
  """
    <div class="ag_repo">
      <div class="blue_subheader ag_repo_name">${repo["name"]}</div>
      <div class="ag_repo_body">
        ${[format_commit(commit) for commit in repo["commits"]]}
      </div>
    </div>
  """

@stringfunction
def format_post(data):
  """
    <div class="ag">
      ${[format_repo(repo) for repo in data]}
    </div>
  """

if __name__ == "__main__":
  projects = simplejson.load(open("/srv/http/projects.json"))["projects"]
  since = simplejson.load(open("/srv/http/commit.json"))["since"]
  new_since = since

  final = []
  
  for repository in projects:
    print (repository)
    commit_list = []
    request = httplib.HTTPSConnection("api.github.com");
    request.putrequest("GET",("/repos/Exeter/%s/commits" % repository["name"]) + 
                    "?client_id=a951833eb1496c8c32ef" +
                    "&client_secret=f338d8a20721decdae676e58c69a127aafdadafc"+
                    ("&since=%s" % since));
    request.putheader("User-Agent", "dabbler0")
    request.endheaders()
    loaded = simplejson.loads(request.getresponse().read());
    for commit in loaded:
      commit_list.append({
        "committer":(commit["author"]["login"] if (commit["author"]) else commit["commit"]["author"]["name"]),
        "user_link":(commit["author"]["html_url"] if (commit["author"]) else "mailto:%s" % commit["commit"]["author"]["email"]),
        "timestamp":commit["commit"]["author"]["date"],
        "link":commit["html_url"],
        "message":commit["commit"]["message"]
      })
      if commit["commit"]["committer"]["date"] > new_since:
         new_since = commit["commit"]["committer"]["date"]
    final.append({
      "name": repository["name"],
      "commits": commit_list
    })
  if len(final) > 0:
    conn = sqlite3.connect("/home/anthony/Projects/new_ecc/news.db")
    cur = conn.cursor()
    cur.execute("INSERT INTO news (timestamp, title, body) VALUES (?, 'Github Commits %s', ?)" % time.strftime("%b %d"), (time.time(), format_post(final)))
    conn.commit()
    conn.close()
    record = open("/srv/http/commit.json","w")
    record.write(simplejson.dumps({"since":(dateutil.parser.parse(new_since) + datetime.timedelta(0,1)).isoformat()[:20]}))
    record.close()
