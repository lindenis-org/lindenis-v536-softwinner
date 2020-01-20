#!/bin/bash

pwd=$1

if [ "$pwd" == "" ];then
    pwd=$PWD
fi

file=$pwd/build_info.lua

date=$(date "+%F %T" | sed 's/ /\ /g')
server=$(ifconfig eth1 | grep -w inet | cut -d ':' -f 2 | cut -d ' ' -f 1)
who=$(whoami)
version=v0.2

# build feature
rtsp=$(grep ENABLE_RTSP $pwd/../app_config.mk | cut -d '=' -f 2)
onvif=$(grep ONVIF_SUPPORT $pwd/../app_config.mk | cut -d '=' -f 2)
tutk=$(grep TUTK_SUPPORT $pwd/../app_config.mk | cut -d '=' -f 2)
gui=$(grep GUI_SUPPORT $pwd/../app_config.mk | cut -d '=' -f 2)
bluetooth=$(grep BT_SUPPORT $pwd/../app_config.mk | cut -d '=' -f 2)
repository=($(repo forall -c "git remote -v | grep gerrit | head -n 1 | cut -d '.' -f 3 | xargs basename"))
commit_id=($(repo forall -c "git log -n 1 | grep commit | cut -d ' ' -f 2"))

echo "app_info = {" > $file
echo "    build_time = \"$date\"," >> $file
echo "    build_server = \"$server\"," >> $file
echo "    build_by_who = \"$who\"," >> $file
echo "    build_with_feature = {" >> $file
echo "        rtsp = $rtsp," >> $file
echo "        onvif = $onvif," >> $file
echo "        tutk = $tutk," >> $file
echo "        gui = $gui," >> $file
echo "        bluetooth = $bluetooth," >> $file
echo "    }," >> $file
echo "    version = \"$version\"," >> $file
echo "    commit_id = {" >> $file

declare -i cnt=${#repository[@]}
declare -i i=0

while ((i<cnt))
do
    echo "        ${repository[i]} = \"${commit_id[i]}\"," >> $file
    i=i+1
done

echo "    }," >> $file
echo "}" >> $file
