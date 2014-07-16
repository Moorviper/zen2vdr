#!/bin/sh

if [ -e /tmp/recordscript ]; then
  for i in `cat /tmp/recordscript`; do
    ${i} $1 $2 $3 $4 $5 $6 $7
  done
fi
