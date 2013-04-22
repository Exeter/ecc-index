ecc-index
=========

A repository containing the source code necessary to run the ECC server landing page. It assumes the following:
 - index.html, publish.html, and about.html are in the root path for the http server
 - all binaries compiled from *.cc are in /cgi-bin/index/
 - /cgi-bin/index is writable
 - a database called news.db exists in /cgi-bin/index/, and is initialized with a table containing (rowid INTEGER PRIMARY KEY ASC, timestamp INTEGER, title TEXT, body TEXT) called news (this will be in a runnable script soon; for now just do it manually)
