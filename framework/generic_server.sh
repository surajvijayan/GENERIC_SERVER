#!/bin/sh
#
# This shell script takes care of starting and stopping
# generic_server framework
#
# chkconfig: - 95 05
# description: generic_server provide a framework to deploy plug-ins

# Source function library.
. /etc/rc.d/init.d/functions

# configuration section
ARCH=lin64
CONF_FILE="generic_server.conf"
GENERIC_SERVER_NAME=`basename $0`
SERVER_ROOT="/d0/operator/$GENERIC_SERVER_NAME/server"
# end configuration section

[ -f $SERVER_ROOT/generic_server_$ARCH ] || exit 0

RETVAL=0

start() 
{
	# Start daemons.
	echo -n $"Starting $GENERIC_SERVER_NAME "
	cd "$SERVER_ROOT"
	daemon "$SERVER_ROOT/generic_server_$ARCH" "$GENERIC_SERVER_NAME" "$SERVER_ROOT/$CONF_FILE"   2>/dev/null
	RETVAL=$?
	echo
	if [ $RETVAL -eq 0 ]; then 
	    if [ -x /usr/bin/logger ]; then
			/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME startup succeeded"
	    fi;
	else
	    if [ -x /usr/bin/logger ]; then
			/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME startup failed"
	    fi;
        fi
	return $RETVAL
}

stop() 
{
	# Stop daemons.
	echo -n $"Shutting down $GENERIC_SERVER_NAME"
	killproc "$GENERIC_SERVER_NAME"
	if [ $RETVAL -eq 0 ]; then 
	RETVAL=$?
	echo
	   if [ -x /usr/bin/logger ]; then
	  		/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME shutdown succeeded"
	   fi;
	else
	   if [ -x /usr/bin/logger ]; then
			/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME shutdown failed"
	   fi;
	fi
	return $RETVAL
}

reload() 
{
	# Reload configuration
	echo -n $"Reloading $GENERIC_SERVER_NAME"
	killproc "$GENERIC_SERVER_NAME" -HUP
	if [ $RETVAL -eq 0 ]; then 
	RETVAL=$?
	echo
	   if [ -x /usr/bin/logger ]; then
	  		/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME reload succeeded"
	   fi;
	else
	   if [ -x /usr/bin/logger ]; then
			/usr/bin/logger -t "$GENERIC_SERVER_NAME" "$GENERIC_SERVER_NAME reload failed"
	   fi;
	fi
	return $RETVAL
}

# See how we were called.
case "$1" in
  start)
	start
	;;
  stop)
	stop
	;;
  restart)
	stop
	start
	RETVAL=$?
	;;
  reload)
	reload
	;;
  status)
	status "$GENERIC_SERVER_NAME"
	RETVAL=$?
	;;
  *)
	echo $"Usage: $0 {start|stop|restart|reload|status}"
	exit 1
esac

exit $RETVAL

