#!/bin/bash

[ "$1" == "verbose" ] && LOG=1

if [ ! -e /sys/devices/platform/w83627hf.656/pwm1 ]; then
  exit 1
fi

echo 1 > /sys/devices/platform/w83627hf.656/temp2_type

TEMP=`cat /sys/devices/platform/w83627hf.656/temp2_input`
LASTPWM=-1

while [ true ]; do
  if [ -e /tmp/standby ]; then
    OFFLEVEL=50000
    TOPLEVEL=62000
    SAMPLES=20
    PWMIDLE=15
  else
    OFFLEVEL=50000
    TOPLEVEL=60000
    SAMPLES=10
    PWMIDLE=20
  fi

  TEMP_NOW=`cat /sys/devices/platform/w83627hf.656/temp2_input`
  
  TEMP=$((${TEMP} - ${TEMP} / $((${SAMPLES} -1 )) ))
  TEMP=$((${TEMP} + ${TEMP_NOW} / ${SAMPLES}))

  PWM=0

  PWM=$((((${TEMP} - ${TOPLEVEL}) / 200) + ${PWMIDLE} ))
  if [ ${PWM} -lt 0 ]; then
    PWM=0
  fi
  
  if [ ${TEMP} -le ${TOPLEVEL} -a ${TEMP} -ge ${OFFLEVEL} ]; then
    if [ ${LASTPWM} == 0 ]; then
      echo 30 > /sys/devices/platform/w83627hf.656/pwm1
      sleep 1
    fi
    PWM=${PWMIDLE}
  fi
  
  [ "$LOG" == "1" ] && echo Temp: $TEMP pwm: $PWM

  [ "${PWM}" != "${LASTPWM}" ] && echo ${PWM} > /sys/devices/platform/w83627hf.656/pwm1
  LASTPWM=${PWM}

  sleep 15

done
