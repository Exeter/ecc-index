#!/usr/bin/env python
import http.client
import simplejson
import datetime
import dateutil.parser
import sqlite3

def format_post(data):
  formatted = ""
  for key in data:
    if (len(data[key]) > 0):
      formatted += """
        <div class="autogen_github_repo_wrapper">
          <div class="autogen_github_repo">
            <div class="autogen_github_repo_name">
              Updates to "%s"
            </div>
            <div class="autogen_github_repo_body">
      """ % key
      for commit in data[key]:
        formatted += """
            <div class="autogen_github_commit">
              <div class="autogen_github_message">
                %s
              </div>
              <div class="autogen_github_commit_meta">
                <a href="%s" class="autogen_github_committer">%s</a>
                <a href="%s" class="autogen_github_timestamp">%s</a>
              </div>
            </div>
        """ % (commit["message"], commit["user_link"], commit["committer"], commit["link"], commit["timestamp"])
      formatted += "</div></div></div>"
  if (len(formatted) > 0):
    return formatted
  else:
    return False

def main():
  projects = simplejson.load(open("/srv/http/projects.json"))["projects"]
  since = simplejson.load(open("/srv/http/commit.json"))["since"]
  new_since = since

  final = {}

  for repository in projects:
    print (repository)
    commit_list = []
    request = http.client.HTTPSConnection("api.github.com");
    request.putrequest("GET",("/repos/Exeter/%s/commits" % repository["name"]) + 
                    "?client_id=a951833eb1496c8c32ef" +
                    "&client_secret=f338d8a20721decdae676e58c69a127aafdadafc"+
                    ("&since=%s" % since));
    request.putheader("User-Agent", "Exeter Computing Club Server")
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
    final[repository["name"]] = commit_list

  xml = format_post(final)
  if (xml != False):
    conn = sqlite3.connect("/srv/http/cgi-bin/index/news.db")
    cur = conn.cursor()
    cur.execute("INSERT INTO news (timestamp, title, body) VALUES ((JULIANDAY('now') - 2440587.5)*86400.0, 'Automatic Github Updates', ?)", (xml,))
    conn.commit()
    conn.close()
    open("/srv/http/commit.json","w").write(simplejson.dumps({"since":(dateutil.parser.parse(new_since) + datetime.timedelta(0,1)).isoformat()[:20]}))

main()
