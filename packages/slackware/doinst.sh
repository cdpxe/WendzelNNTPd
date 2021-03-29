#!/bin/sh

config() {
  NEW="$1"
  OLD="`dirname $NEW`/`basename $NEW .new`"
  # If there's no config file by that name, mv it over:
  if [ ! -r $OLD ]; then
    mv $NEW $OLD
  elif [ "`cat $OLD | md5sum`" = "`cat $NEW | md5sum`" ]; then # toss the redundant copy
    rm $NEW
  fi
  # Otherwise, we leave the .new copy for the admin to consider...
}

preserve_perms() {
  NEW="$1"
  OLD="$(dirname ${NEW})/$(basename ${NEW} .new)"
  if [ -e ${OLD} ]; then
    cp -a ${OLD} ${NEW}.incoming
    cat ${NEW} > ${NEW}.incoming
    mv ${NEW}.incoming ${NEW}
  fi
  config ${NEW}
}

# Keep same perms when installing rc.httpd.new:
preserve_perms etc/rc.d/rc.wendzelnntpd.new
preserve_perms var/spool/news/wendzelnntpd/usenet.db.new

# Handle config files.  Unless this is a fresh installation, the
# admin will have to move the .new files into place to complete
# the package installation, as we don't want to clobber files that
# may contain local customizations.
config etc/wendzelnntpd.conf.new
