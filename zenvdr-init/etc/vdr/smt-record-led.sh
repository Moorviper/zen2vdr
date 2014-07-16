#!/bin/sh
if [ -e /tmp/recnum ]; then
  RECORDINGS=`cat /tmp/recnum`
else
  RECORDINGS=0
fi

if [ X${1} == Xbefore ]; then
  RECORDINGS=$(( $RECORDINGS + 1 ))
fi
if [ X${1} == Xafter ]; then
  RECORDINGS=$(( $RECORDINGS - 1 ))
fi

if [ $RECORDINGS == 0 ]; then
  echo -n r > /tmp/dispdata
else
  echo -n R > /tmp/dispdata
fi

echo $RECORDINGS > /tmp/recnum