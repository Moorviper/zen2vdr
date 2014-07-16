#!/bin/bash

. /etc/vdr.conf
. /etc/admin.conf

# Fix the EPG location to tmpfs if desired
[ X${EPG_TMPFS} == X1 ] && VDR_EPG_DATA="/tmp/epg.data"

#
# Scan the vdr.d dir and create a list of plugin and
# config file dirs
#

#export LD_ASSUME_KERNEL="2.4.1"
FIRST_START="YES"

#Process the entire list of available plugins and link active plugins in
rm -f /tmp/savelist

if [ -e /etc/vdr/plugins.conf ]; then
  . /etc/vdr/plugins.conf
  rm /etc/vdr/plugins.conf
fi

[ -e /tmp/admin_plugins.conf ] && rm /tmp/admin_plugins.conf

for i in /etc/vdr.d/all/plg.* ; do
  CONFIG_FILE_SAVE=""
  PLUGIN_PREFFERED_NUMBER=""
  PLUGIN_NAME=""

  . ${i}

  [ "${CONFIG_FILE_SAVE}" != "" ] && echo ${CONFIG_FILE_SAVE} >> /tmp/savelist

  # Prepare to link activated plugins in. Also create a plugin section for the admin plugin
  
  CLEAN_PLUGIN_NAME=`echo $PLUGIN_NAME | tr '-' '_'`
  eval ACTIVATE=\${PLUGIN_$CLEAN_PLUGIN_NAME}

  [ "${PLUGIN_PREFERRED_NUMBER}" == "" ] && PLUGIN_PREFERRED_NUMBER=0200

  if [ X${ACTIVATE} == X ]; then
    if [ -e /etc/vdr.d/${PLUGIN_PREFERRED_NUMBER}-plg.${PLUGIN_NAME} ]; then
      ACTIVATE=1
    else
      ACTIVATE=0
    fi      
  fi

  # in failsafe mode we only load the default plugins
  if [ -e /tmp/failsafe -a "${PLUGIN_NAME}" != "admin" -a "${PLUGIN_NAME}" != "xineliboutput" ]; then
    ACTIVATE=0
  fi

  if [ X${ACTIVATE} == X1 ]; then
    ln -sf $i /etc/vdr.d/${PLUGIN_PREFERRED_NUMBER}-plg.${PLUGIN_NAME}
    
    PLUGIN_FN=`echo ${PLUGIN_NAME} | awk -F "-" '{ print $1 }'`
    [ -e /tmp/config/updates/plg-${PLUGIN_FN}* ] && tar xzpf /tmp/config/updates/plg-${PLUGIN_FN}* --exclude "./etc/vdr.d"
    [ -e /storage/updates/plg-${PLUGIN_FN}* ] && tar xzpf /storage/updates/plg-${PLUGIN_FN}* --exclude "./etc/vdr.d"
    
  else
    rm -f /etc/vdr.d/${PLUGIN_PREFERRED_NUMBER}-plg.${PLUGIN_NAME}
  fi

  echo PLUGIN_${CLEAN_PLUGIN_NAME}=${ACTIVATE} >> /etc/vdr/plugins.conf
  echo "${VDR_CONFIG_DIR}/plugins.conf:PLUGIN_${CLEAN_PLUGIN_NAME}:${ACTIVATE}:B:${ACTIVATE}:Nein,Ja:${PLUGIN_NAME}:" >> /tmp/admin_plugins.conf
done

# Make sure to update the config files after they become visible 
[ -e /tmp/config/conf/vdr-config.tgz ] && cp /tmp/config/conf/vdr-config.tgz /tmp

# Restore the last plugin configuration
tar xzf /tmp/vdr-config.tgz -C ${VDR_CONFIG_DIR}
rm /tmp/vdr-config.tgz

while true; do
  rm -f /tmp/runvdr
  rm -f /tmp/prescript
  rm -f /tmp/postscript
  rm -f /tmp/recordscript
  rm -f /tmp/restartscript

  ALL_PLG_PARMS=""
  ALL_VDR_PARMS=""

  for i in /etc/vdr.d/????-* ; do
    RECORD_SCRIPT=""
    STARTUP_SCRIPT=""
    RESTART_SCRIPT=""
    PLUGIN_NAME=""
    PLUGIN_PARAMETERS=""
    VDR_PARAMETERS=""
    SHUTDOWN_SCRIPT=""
  
    . ${i}
  
    # Setup this 
  
    [ "${STARTUP_SCRIPT}"   != "" ] && echo ${STARTUP_SCRIPT}   >> /tmp/prescript
    [ "${SHUTDOWN_SCRIPT}"  != "" ] && echo ${SHUTDOWN_SCRIPT}  >> /tmp/postscript
    [ "${RESTART_SCRIPT}"   != "" ] && echo ${RESTART_SCRIPT}   >> /tmp/restartscript
    [ "${RECORD_SCRIPT}"    != "" ] && echo ${RECORD_SCRIPT}    >> /tmp/recordscript
    [ "${PLUGIN_NAME}"      != "" ] && ALL_PLG_PARMS="$ALL_PLG_PARMS -P \"${PLUGIN_NAME} ${PLUGIN_PARAMETERS}\""
    [ "${VDR_PARAMETERS}"   != "" ] && ALL_VDR_PARMS="$ALL_VDR_PARMS ${VDR_PARAMETERS}\""
  done

  echo "LANG=${VDR_START_LANG} \\" > /tmp/runvdr
  echo "VDR_CHARSET_OVERRIDE=${VDR_START_CHARSET_OVERRIDE} \\" >> /tmp/runvdr
  echo "$VDR_EXEC \\" >> /tmp/runvdr
  echo "-c ${VDR_CONFIG_DIR} \\" >> /tmp/runvdr
  echo "-L ${VDR_LIB_DIR} \\" >> /tmp/runvdr
  echo "-r ${VDR_RECORD_CTL} \\" >> /tmp/runvdr
  echo "-E ${VDR_EPG_DATA} \\" >> /tmp/runvdr
  echo "-v ${VDR_VIDEO_DIR} \\" >> /tmp/runvdr
  echo "--no-kbd \\" >> /tmp/runvdr
  
  . /etc/adminbase.conf  
  [ "${VIDEO_MOUNT_TYPE}" == "vfat" ] && echo "--vfat \\" >> /tmp/runvdr

  echo $ALL_VDR_PARMS $ALL_PLG_PARMS $LOG_OUTPUT >> /tmp/runvdr

  if [ ${FIRST_START} == YES ]; then
    FIRST_START="NO"
    [ -e /tmp/prescript ] &&  . /tmp/prescript
  fi
  
  touch /tmp/starttime
  
  . /tmp/runvdr
  RC=$?

  # End if we are called by shutdown
  if [ -e /tmp/shutdown ]; then
    break
  fi

  # Restart if config was altered 
  if [    /etc/admin.conf -nt /tmp/starttime \
       -o /etc/adminbase.conf -nt /tmp/starttime \
       -o /etc/vdr/plugins/admin/admin.conf -nt /tmp/starttime ]; then
    touch /tmp/vdrrestart
    break
  fi
  
  # End if VDR wants to end.
  if [ $RC == 0 ]; then
    break
  fi
    
  [ -e /tmp/restartscript ] &&  . /tmp/restartscript
  sleep 2
done

# Save all the relevant files that need to survive a shutdown 
if [ -e /tmp/savelist ]; then
  ( cd ${VDR_CONFIG_DIR} ; tar czf /tmp/vdr-config.tgz `cat /tmp/savelist` ) 
else
  rm /tmp/vdr-config.tgz
fi

if [ -e /tmp/postscript ]; then
  . /tmp/postscript
fi

[ -e /tmp/vdrrestart -a ! -e /tmp/failsafe -a ! -e /tmp/maintenance ] && /sbin/reboot
