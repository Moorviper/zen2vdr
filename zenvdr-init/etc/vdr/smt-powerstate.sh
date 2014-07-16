#!/bin/bash

RC=0
SHUTDOWN=0

if [ -e /tmp/pwlock ]; then
  exit 1
fi

touch /tmp/pwlock

. /etc/admin.conf

# Quick exit if standby is disabled
[ "${NO_STANDBY}" == "1" ] && touch /tmp/shutdown && /sbin/halt && rm /tmp/pwlock && exit 0

#Save the VDR config files ( just to be sure )
[ ! -e /tmp/config/conf/etc/vdr ] && mkdir -p /tmp/config/conf/etc/vdr
for i in /etc/vdr/*.conf; do
  [ -e ${i} ] && cp ${i} /tmp/config/conf/etc/vdr
done

if [ X$1 == Xpoweroff ]; then
  if [ -e /etc/vdr/timers.conf ]; then
    CTACTIVE=0
    while read line
      do
      ACTIVE=`echo ${line} | awk -F : '{ print $1 }'`
      if [ X${ACTIVE} == X1 ]; then
        CTACTIVE=$((${CTACTIVE} + 1))
      fi
    done < /etc/vdr/timers.conf
                          
    if [ X${CTACTIVE} == X0 ]; then
      SHUTDOWN=1
    fi
  else
    SHUTDOWN=1
  fi
fi

if [ ${SHUTDOWN} == 1 ]; then
  #stop all unneeded processes
  touch /tmp/shutdown
  /sbin/halt
  exit 0
fi

if [ ! -e /tmp/standby ]; then
  echo S > /tmp/dispdata
  [ ! -e /tmp/maintenance ] && /usr/sbin/stv6421tool -s
  touch /tmp/standby
  #stop all unneeded processes
  killall vdr-fbfe
  [ X${VIDEO_DEVICE} == Xdisk ] && hdparm -S 48 /dev/hda
else
  echo s > /tmp/dispdata
  rm /tmp/standby
  [ X${VIDEO_DEVICE} == Xdisk ] && hdparm -S 0 /dev/hda
  killall sleep
  [ ! -e /tmp/maintenance ] && /usr/sbin/stv6421tool -o
fi

rm /tmp/pwlock
