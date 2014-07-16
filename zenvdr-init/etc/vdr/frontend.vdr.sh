#!/bin/bash

. /etc/vdr/frontend.conf

ASPECT="4:3"
grep -q "VideoFormat = 1" /etc/vdr/setup.conf && ASPECT="16:9"

if [ -e /dev/cgroup/frontend ]; then
  echo $$ > /dev/cgroup/frontend/tasks
fi

/usr/local/bin/vdr-fbfe --video=directfb --audio=alsa --lirc --nokbd --fullscreen --aspect=${ASPECT} ${FRONTEND_BUFFERS} ${FRONTEND_METHOD} ${FRONTEND_URL} &> /tmp/fbfe.log
