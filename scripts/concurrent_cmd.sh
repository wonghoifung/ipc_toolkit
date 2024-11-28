#!/bin/sh

loop() {
    while true; do
        $@
        sleep 1
    done
}

max=5
counter=0
while [ $counter -lt $max ]; do
    loop $@ &
    counter=$((counter + 1))
done

echo "start $max concurrent workers..."

# while true; do
#     sleep 1
# done
