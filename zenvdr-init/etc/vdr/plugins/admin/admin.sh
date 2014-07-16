#!/bin/bash
logger -s "$0 $1"

. /etc/vdr.conf

if [ "$1" = "-init" ] ; then
  echo "vdr starting"
  
  echo "(re)building admin.conf"
   
  while read line; do 
    echo ${line} | grep -q :Features && break
    echo ${line} >> /tmp/admin_base.conf
  done < ${VDR_CONFIG_DIR}/plugins/admin/admin.conf   
   
  echo ":Features" >> /tmp/admin_base.conf
  cat /tmp/admin_features.conf >> /tmp/admin_base.conf

  echo ":Plugins" >> /tmp/admin_base.conf
  cat /tmp/admin_plugins.conf >> /tmp/admin_base.conf
   
  mv /tmp/admin_base.conf ${VDR_CONFIG_DIR}/plugins/admin/admin.conf
elif [ "$1" = "-start" ] ; then
  echo "admin plugin startet"
elif [ "$1" = "-save" ] ; then
  /etc/vdr/plugins/admin/commit_conf.sh
else
  echo "Illegal Parameter <$1>"
fi
