#!/bin/sh

mkdir /var/log

killall syslogd
killall klogd

/usr/bin/touch /tmp/maintenance
