ecc-index
=========

A repository containing the source code necessary to run the ECC server landing page. It assumes the following:
 - index.html, publish.html, and about.html are in the root path for the http server
 - a writable json file called projects.json is in root (intially empty)
 - a writable json file called commit.json is in root (begin with {"since":"0000-00-00T00:00:00Z"})
 - all binaries compiled from *.cc are in /cgi-bin/index/
 - /cgi-bin/index is writable
 - a database called news.db exists in /cgi-bin/index/, and is initialized with a table containing (rowid INTEGER PRIMARY KEY ASC, timestamp INTEGER, title TEXT, body TEXT) called news (this will be in a runnable script soon; for now just do it manually)

Additionally, you must run commit_cron.py and project_cron.py at least daily.

###TODO:
 - Add collapsible news entries
