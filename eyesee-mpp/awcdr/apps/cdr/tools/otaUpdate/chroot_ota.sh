#!/bin/sh
#
# will change rootfs to dram
# this script was used to ota_update
#

#if result isn't 0, mean some thing was erro
res=0
update_dir=""

#check if sdcard was be mout
if [ "`mount | grep "/mnt/extsd"`" ]; then
	#the sdcard was be mount, so we use sdcard to tmp dir of update
	rm -rf /mnt/extsd/.update
	mkdir -p /mnt/extsd/.update
	let res=$res+$?
	update_dir="/mnt/extsd/.update"
else
	echo "sdcard wasn't mount in /mnt/extsd, please mount sdcard and try again"
	exit -1
	
fi

#cpoy rootfs to $update_dir and mount it
rm -rf $update_dir/rootfs
mkdir -p $update_dir/rootfs
let res=$res+$?
rm -rf $update_dir/rootfs_old
dd if=/dev/mtd_name/rootfs of=$update_dir/rootfs_old
let res=$res+$?
losetup -d /dev/loop0
losetup /dev/loop0 $update_dir/rootfs_old
let res=$res+$?
mount /dev/loop0 $update_dir/rootfs
let res=$res+$?

#re-mout proc and sys to new rootfs
mount -t proc none $update_dir/rootfs/proc
let res=$res+$?
mount -t sysfs none $update_dir/rootfs/sys
let res=$res+$?
mount -t tmpfs none $update_dir/rootfs/tmp/
let res=$res+$?
mount -t devtmpfs none $update_dir/rootfs/dev
let res=$res+$?
echo "1the result is $res"

#mount img dir to new rootfs in /mnt/extsd
mount --bind $1 $update_dir/rootfs/mnt/extsd/
let res=$res+$?

if [ $res -eq 0 ]; then
	echo "ready to chroot and running app"
else
	echo "some wrong,please checking"
	exit $res
fi

#put app to new rootfs that in dram.and running
chroot $update_dir/rootfs /usr/bin/ota_update /mnt/extsd/
let res=$res+$?

if [ $res -eq 0 ]; then
	echo "success change rootfs to dram"
else
	echo "change rootfs fail"
fi

exit $res

