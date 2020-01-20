#!/bin/bash

pwd=$1

if [ "$pwd" == "" ];then
    pwd=$PWD
fi

# generate boot script use for launch app
cat > ${pwd}/S01app << EOF
#!/bin/sh

MODULES_DIR="/lib/modules/\`uname -r\`"

mkdir -p /tmp/event/

echo 100 > /proc/sys/vm/dirty_writeback_centisecs
echo 50 > /proc/sys/vm/dirty_expire_centisecs
export FB_SYNC='1'
export SCREEN_INFO='480x640-32bpp'
export TERM='xterm'
export TZ='Asia/Shanghai'
export LD_HWCAP_MASK='0x00000000'

if [ ! -e "/tmp/data" ];then
    if [ -d "/data" ];then
        ln -s /data /tmp/data
    fi
fi

insmod \${MODULES_DIR}/hdmi.ko

start() {
    #adjust dram
    echo "12 3" > /sys/class/hwmon/hwmon0/port_qos
    echo "0 2" > /sys/class/hwmon/hwmon0/port_qos
    echo "2 2" > /sys/class/hwmon/hwmon0/port_qos
    echo "4 2" > /sys/class/hwmon/hwmon0/port_qos
    echo "11 2" > /sys/class/hwmon/hwmon0/port_qos
    echo "12 4096" > /sys/class/hwmon/hwmon0/port_bwl1
    echo '0x04002398 0x80c00004' > /sys/class/sunxi_dump/write
    echo '0x040023b8 0x80c00004' > /sys/class/sunxi_dump/write
    #/usr/bin/tinymix 9 160
    #/usr/bin/tinymix 10 160
    #/usr/bin/tinymix 21 1
    #/usr/bin/tinymix 49 1
    #/usr/bin/tinymix 53 1
    #/usr/bin/tinymix 55 1
    #/usr/bin/tinymix 60 1
    #/usr/bin/tinymix 13 25

    if [ -z "\$(grep '\<vfat\>' /proc/mounts)" ]; then
        ! /bin/mount -t vfat /dev/mmcblk0 /mnt/extsd 2>/dev/null &&
        /bin/mount -t vfat /dev/mmcblk0p1 /mnt/extsd
    fi
	/usr/bin/dragonboard &
    hwclock --hctosys
}

stop() {

    sleep 1
}

case "\$1" in
    start)
        start
        ;;
    stop)
        stop
        ;;
    restart|reload)
        stop
        start
        ;;
    *)
        echo "Usage: \$0 {start|stop|restart}"
        exit 1
esac

exit \$?

EOF

chmod 755 ${pwd}/S01app

# generate network config script
#cat > $LICHEE_EYESEE_BR_OUT/target/etc/init.d/auto_config_network << EOF


#MAC_ADDR="\`cat /proc/cpuinfo | awk '\$1=="Serial" {print \$3}' | sed 's/../&:/g' | cut -c16-29\`"

#ifconfig eth0 hw ether "00:\$MAC_ADDR"
#ifconfig lo 127.0.0.1
#udhcpc -b -R &

#EOF
#chmod 755 $LICHEE_EYESEE_BR_OUT/target/etc/init.d/auto_config_network

# update /etc/shadow to generate root password
#password="admin"
#shadow=$(echo $password | $pwd/mkpasswd -s)
#sed -i "s%^root::%root:$shadow:%" $LICHEE_EYESEE_BR_OUT/target/etc/shadow
