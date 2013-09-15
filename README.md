ecc-index
=========

Source code necessary to run the ECC landing page. Clone into /home/daemon/ecc-index/, then cp manifest.txt /srv/http/manifest.txt; cp mime.types /srv/http/conf/mime.types; ./py/repo_cron.py; apxs -iac manifester.c; mkdir db; touch db/auth.db db/news.db db/users.db.

You'll need to add one user manually at first.

Depends on Python 2.7, Apache 2.4, and pycrypto.
