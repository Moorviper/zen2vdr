#!/bin/bash
while [ ! -e /tmp/shutdown ]; do
  /usr/bin/clear > /dev/tty0
  
  while [ -e /tmp/standby ]; do
    sleep 3600
  done

  /etc/vdr/frontend.vdr.sh
done
