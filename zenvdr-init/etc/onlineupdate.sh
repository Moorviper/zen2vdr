#!/bin/sh

. /etc/update.conf

cd /tmp

wget -q http://${UPDATE_SERVER}/zentoo/${UPDATE_VERSION}/VERSION

if [ $? != 0 ]; then
  echo Download failed.
  exit 1
fi

if [ 0`cat VERSION` -le 0`[ -e /VERSION ] && cat /VERSION` ]; then 
  echo "System is up-to-date."
  exit 1
fi

echo Update available system will update now ...

touch /tmp/config/flashupdate
[ X$1 == Xclear ] && touch /tmp/config/flashupdate_clear

[ X$1 != noreboot ] && reboot

[ X$2 == poweroff ] && /tmp/config/flashupdate_shutdown