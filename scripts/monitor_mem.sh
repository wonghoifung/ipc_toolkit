#!/bin/sh

prog=$1

if [ "$prog" = "" ]; then
    echo "usage: monitor_mem.sh <prog-name>"
    return 1
fi

last=
while true; do
    current=$(cat /proc/$(pgrep $prog)/status | grep RSS)
    if [ "$last" != "$current" ]; then
        echo $current
        last=$current
    fi
    sleep 1
done

