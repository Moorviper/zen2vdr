#!/bin/bash
. $1

$OTHERS=`ps xa | grep wget | grep -v $URL | awk '{ print $1 }'`

[ "$OTHERS" == "" ] || kill $OTHERS

wget -q -O $2 "$URL" &> /tmp/wget.log
