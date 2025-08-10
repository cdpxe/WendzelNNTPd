#!/bin/bash -e

DATA_DIR=/var/spool/news/wendzelnntpd
DB=$DATA_DIR/usenet.db

# Cleanup data directory
rm $DATA_DIR/*

# Initialize database structure
cat ../database/usenet.db_struct | sqlite3 $DB

# Create test data in the database:
# 3 newgroups: "alt.wendzelnntpd.test", "alt.wendzelnntpd.test.empty" and "alt.wendzelnntpd.test.post"
# 2 postings in newsgroup "alt.wendzelnntpd.test"
# 1 user with the name "testuser" and password "password"
cat create_db_test_data.sql | sqlite3 $DB

# Copy existing posting
cp test-files/cdp?@localhost $DATA_DIR/

# Initialize nextmsgid with 2, because there are already 2 postings in the database
printf '\x02\x00\x00\x00\x00\x00\x00\x00' > $DATA_DIR/nextmsgid
