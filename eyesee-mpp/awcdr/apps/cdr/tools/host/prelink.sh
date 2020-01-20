#!/bin/bash

pwd=$1

if [ "$pwd" == "" ];then
    pwd=$PWD
fi
cp $pwd/prelink.conf $TARGET_OUT/target/etc
mkdir -p $TARGET_OUT/target/etc/cache
$pwd/prelink -avR --root=$TARGET_OUT/target --cache-file=/etc/cache/prelink.cache --config-file=/etc/prelink.conf -h /usr/bin/sdvcam 1>&2
