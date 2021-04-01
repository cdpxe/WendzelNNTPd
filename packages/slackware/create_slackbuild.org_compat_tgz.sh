#!/bin/bash -e

mkdir wendzelnntpd
cp doinst.sh README slack-desc wendzelnntpd.info  wendzelnntpd.SlackBuild wendzelnntpd.url wendzelnntpd/
tar -czvf wendzelnntpd.tar.gz wendzelnntpd/*
rm -rf wendzelnntpd
ls -lh wendzelnntpd.tar.gz

