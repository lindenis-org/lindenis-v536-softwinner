#!/bin/sh

ROOTFS=${TARGET_OUT}/target
SENSOR_NAME=${SENSOR_NAME}
STRIP=arm-linux-gnueabi-strip

# reduce dynamic library size
reduce_shared_lib() {
    find_libs="`find ${ROOTFS} -name "*so*"`"

    for lib in ${find_libs}; do
        file_desc="`file ${lib} | grep "shared object" | grep "ARM" | grep "ELF"`"
        if [ "${file_desc}" != "" ] && [ ! -L ${lib} ]; then
            ${STRIP} ${lib}
        fi
    done
}

# reduce executable binary file size
reduce_bin_file() {
    find_bins="`find ${ROOTFS} | grep "bin"`"

    for bin in ${find_bins}; do
        file_desc="`file ${bin} | grep "LSB executable" | grep "ELF" | grep "ARM"`"
        if [ "${file_desc}" != "" ] && [ ! -L ${bin} ]; then
            ${STRIP} ${bin}
        fi
    done
}

# kernel modules
rm -vf ${ROOTFS}/lib/modules/4.9.118/actuator.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/ar0238.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/drbg.ko
# rm -vf ${ROOTFS}/lib/modules/4.4.55/sunxi_ise.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/hmac.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/sha256_generic.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/sunxi_eise.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/sunxi_ise.ko
#rm -vf ${ROOTFS}/lib/modules/4.9.118/imx317_mipi.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/jitterentropy_rng.ko
rm -vf ${ROOTFS}/lib/modules/4.9.118/imx386_mipi.ko


# others
rm -vf ${ROOTFS}/THIS_IS_NOT_YOUR_ROOT_FILESYSTEM
rm -vf ${ROOTFS}/usr/bin/tar
rm -vf ${ROOTFS}/usr/bin/openssl
rm -vf ${ROOTFS}/usr/bin/server
rm -vf ${ROOTFS}/usr/bin/server_shared
rm -vf ${ROOTFS}/usr/bin/sadf
rm -vf ${ROOTFS}/usr/bin/sar
rm -vf ${ROOTFS}/bin/bash
rm -vf ${ROOTFS}/usr/bin/gdb
rm -vf ${ROOTFS}/usr/bin/gdbserver
rm -vf ${ROOTFS}/usr/bin/gcore
rm -vf ${ROOTFS}/usr/bin/htop
rm -vf ${ROOTFS}/usr/bin/iozone
rm -vf ${ROOTFS}/usr/share/osd/fonts/asc16.lz4
rm -vf ${ROOTFS}/usr/share/osd/fonts/hzk16.lz4
rm -vf ${ROOTFS}/usr/share/osd/fonts/hzk32.lz4
rm -vf ${ROOTFS}/usr/share/osd/fonts/hzk64.lz4
cd ${ROOTFS}/usr/share/osd/fonts/ && ln -s asc32.lz4 hzk32.lz4 && cd - > /dev/null
cd ${ROOTFS}/usr/share/osd/fonts/ && ln -s asc64.lz4 hzk64.lz4 && cd - > /dev/null
#rm -vf ${ROOTFS}/usr/sbin/wpa_supplicant
#rm -vf ${ROOTFS}/usr/sbin/wpa_passphrase
rm -vf ${ROOTFS}/usr/bin/ts_*
rm -vf ${ROOTFS}/usr/bin/aserver
rm -vf ${ROOTFS}/usr/bin/civetweb
rm -vf ${ROOTFS}/usr/bin/luac
rm -vf ${ROOTFS}/usr/bin/strace
rm -vf ${ROOTFS}/usr/bin/strace-log-merge
rm -vf ${ROOTFS}/usr/bin/nfsmount
rm -vf ${ROOTFS}/usr/bin/hostapd_cli
#rm -vf ${ROOTFS}/usr/bin/log_guardian
#rm -vf ${ROOTFS}/usr/bin/webapp

# This delete opr is for reduce physical memory use
[ -e ${ROOTFS}/etc/init.d/S01logging ] && {
    rm ${ROOTFS}/etc/init.d/S01logging
}

[ -e ${ROOTFS}/etc/init.d/S50telnet ] && {
    rm ${ROOTFS}/etc/init.d/S50telnet
}

if [ "${BOARD_BOOT_TYPE}" = "fast" ]; then
    org_size=`du -b --max-depth=0 ${ROOTFS} | awk {'printf $1'}`

    [ -e ${ROOTFS}/etc/init.d/S00appdriver ] && {
        rm ${ROOTFS}/etc/init.d/S00appdriver
    }
    [ -e ${ROOTFS}/etc/init.d/S00mpp ] && {
        rm ${ROOTFS}/etc/init.d/S00mpp
    }

    rm -vf ${ROOTFS}/lib/modules/4.4.55/Module.symvers
    rm -vf ${ROOTFS}/lib/modules/4.4.55/imx278.ko
#    rm -vf ${ROOTFS}/lib/modules/4.4.55/imx317_mipi.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/videobuf2-dma-contig.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/vin_io.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/vin_v4l2.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/sunxi_gpadc.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/ft6x-ts.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/industrialio-triggered-buffer.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/inv-mpu6050.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/sunxi_aio.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/sunxi_ise.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/actuator.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/an41908a_act.ko
    rm -vf ${ROOTFS}/lib/modules/4.4.55/ov8825_act.ko

    ${TARGET_TOP}/custom_aw/products/fast-boot/reduce_shared_libs.sh
    reduce_shared_lib
    reduce_bin_file

    new_size=`du -b --max-depth=0 ${ROOTFS} | awk {'printf $1'}`
    reduce_size="(${org_size})->(${new_size})Bytes"
    echo "reduce ${reduce_size}"
fi
