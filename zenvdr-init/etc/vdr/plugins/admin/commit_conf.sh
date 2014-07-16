#!/bin/bash

echo "Committing config."

FILES=""

while read line; do 
  echo ${line} | grep -q / || continue

FILE=`echo ${line} | awk -F : '{ print $1}'`
VAR=`echo ${line} | awk -F : '{ print $2}'`
VAL=`echo ${line} | awk -F : '{ print $3}'`

if grep -q ^${VAR} ${FILE} ; then
  cat ${FILE} | sed  "s/${VAR}[^$]*/${VAR}=\"${VAL}\"/" > /tmp/commit.tmp
  mv /tmp/commit.tmp ${FILE}
else
  echo \# Auto-inserted variable. Consider upgrading. >> ${FILE}
  echo ${VAR}=\"${VAL}\" >> ${FILE}
fi

done < /etc/vdr/plugins/admin/admin.conf