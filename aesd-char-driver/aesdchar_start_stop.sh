#!/bin/sh

case "$1" in
start)
    echo "Starting aesdchar module"
    cd /lib/modules/5.15.124-yocto-standard/extra/ && ./aesdchar_load
    ;;
    
stop)
    echo "Stopping scull module"
    cd /lib/modules/5.15.124-yocto-standard/extra/ && ./aesdchar_unload
        ;;
*)
    echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0