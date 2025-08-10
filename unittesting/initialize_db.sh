#!/bin/bash -e

DATA_DIR=/var/spool/news/wendzelnntpd
DB=$DATA_DIR/usenet.db

# Cleanup data directory
rm $DATA_DIR/*

# Initialize database structure
sqlite3 $DB < ../database/usenet.db_struct

# Create test data in the database:
# 3 newgroups: "alt.wendzelnntpd.test", "alt.wendzelnntpd.test.empty" and "alt.wendzelnntpd.test.post"
# 2 postings in newsgroup "alt.wendzelnntpd.test"
# 1 user with the name "testuser" and password "password"
sqlite3 $DB < create_db_test_data.sql

# Copy existing posting
cp test-files/cdp?@localhost $DATA_DIR/

# Initialize nextmsgid with 2, because there are already 2 postings in the database
printf '\x02\x00\x00\x00\x00\x00\x00\x00' > $DATA_DIR/nextmsgid
