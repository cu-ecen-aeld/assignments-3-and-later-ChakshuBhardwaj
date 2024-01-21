#! /bin/sh

case "$1" in
start)
    echo "Starting aesdsocket"
    start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdsocket -- -d
    #start-stop-daemon --start --background --exec /usr/bin/aesdsocket
    ;;
    
stop)
    echo "Stopping aesdsocket"
    start-stop-daemon -K -n aesdsocket
    #start-stop-daemon --stop --exec /usr/bin/aesdsocket
        ;;
*)
    echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0