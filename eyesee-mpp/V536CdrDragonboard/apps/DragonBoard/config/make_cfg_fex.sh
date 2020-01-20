#!/bin/bash

pwd=$1

if [ "$pwd" == "" ];then
    pwd=$PWD
fi

MKJFFS2=$pwd/mkfs.jffs2
CFG_PATH="$LICHEE_DIR/tools/pack/chips/sun8iw12p1/configs/v5s_pro_lpddr3"
PARTITION_SIZE=`grep -h "name.*overlay" -A 4 $CFG_PATH/sys_partition_nor.fex | grep size | awk '{print $3}'`

PAD_SIZE=0x$(echo "obase=16; $[$PARTITION_SIZE*512]" | bc)

# -p total size
mkdir -p $pwd/overlay/upperdir
mkdir -p $pwd/overlay/workdir
$MKJFFS2 -d $pwd/overlay -s 0x100 -p $PAD_SIZE -o $pwd/overlay.fex

mv $pwd/overlay.fex $CFG_PATH
