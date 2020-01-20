#!/bin/sh

ROOTFS=${TARGET_OUT}/target
OUTPUT=${TARGET_OUT}
FAST_BOOT_TOOLS=${TARGET_TOP}/custom_aw/products/fast-boot

READELF=arm-linux-gnueabi-readelf

exclude_libs_txt=${OUTPUT}/.exclude_libs_txt_tmp.txt
needed_lib_txt=${OUTPUT}/.needed_libs_tmp.txt
sorted_lib_txt=${OUTPUT}/.sorted_libs_tmp.txt

parse_needed_libs() {
    find_bins="`find ${ROOTFS} | grep "bin/"`"
    find_libs="`find ${ROOTFS} | grep "\.so"`"
    find_all="`find ${ROOTFS} | grep -E "bin/|\.so"`"

    >${needed_lib_txt}

    for elf in ${find_all}; do
        file_desc="`file ${elf} | grep "ELF" | grep "ARM"`"

        if [ "x${file_desc}x" != "xx" ]; then
            if [ ! -L ${elf} ]; then
                real_elf=${elf}
            else
                real_elf="`readlink -f ${elf}`"
            fi
            needlibs="`${READELF} -d ${real_elf} | grep "NEEDED" | awk '{print $NF}'`"
            echo "[${elf##*target}]" >> ${needed_lib_txt}
            for needed in ${needlibs}; do
                tmp=${needed##*\[}
                tmp_strip=${tmp%%\]*}
                echo ${tmp_strip} >> ${needed_lib_txt}
            done
        fi
    done

    sort ${needed_lib_txt} | grep -v "\[" | uniq > ${sorted_lib_txt}

    # judge if the libs are used libs
    sort_libs="`cat ${sorted_lib_txt}`";
    for lib in ${find_libs}; do
        exist_lib=${lib##*/}
        exist_lib=${exist_lib%%.so*}
        for sort_lib in ${sort_libs}; do
            sort_lib=${sort_lib%%.so*}
            if [ "${exist_lib}" = "${sort_lib}" ]; then
                if [ -L ${lib} ]; then
                    echo "`readlink -f ${lib}`" >> ${exclude_libs_txt}
                fi
                echo ${lib} >> ${exclude_libs_txt}
            fi
        done
    done

    # now, delete all noused real lib files
    exclude_libs="`cat ${exclude_libs_txt}`";
    for lib in ${find_libs}; do
        exist_lib=${lib##*/}
        used_flag="unused"
        exist_lib_dir=${lib##*target/}
        for keep_lib in ${exclude_libs}; do
            tmp_lib=${keep_lib##*/}

            if [ -d ${keep_lib} ]; then
                tmp_lib_dir=${keep_lib##*target//}
                tmp_lib_dir=${tmp_lib_dir##*target/}
                tmp_lib_dir="`echo ${exist_lib_dir} | grep "${tmp_lib_dir}"`"
                if [ "x${tmp_lib_dir}x" != "xx" ]; then
                    used_flag="used"
                fi
            elif [ "${exist_lib}" = "${tmp_lib}" ]; then
                used_flag="used"
            fi
        done
        if [ "${used_flag}" = "unused" ]; then
            if [ ! -L ${lib} ]; then
                # delete it
                rm ${lib}
                echo ${lib##*target/}
            fi
        fi
    done

    for lib in ${find_libs}; do
        if [ -L ${lib} ]; then
            # if is invalid link, then delete it
            real_lib="`readlink -f ${lib}`"
            if [ ! -e ${real_lib} ]; then
                # delete it
                rm ${lib}
                echo ${lib##*target/}
            fi
        fi
    done

    rm ${needed_lib_txt}
    rm ${sorted_lib_txt}
    rm ${exclude_libs_txt}
}

>${exclude_libs_txt}
pre_exclude_libs() {
    pre_ex_files="`cat ${FAST_BOOT_TOOLS}/pre_exclude_libs.txt | grep -v "^#"`"
    for ex_file in ${pre_ex_files}; do
        if [ "x${ex_file}x" != "xx" ]; then
            echo "${TARGET_OUT}/target/${ex_file}" >> ${exclude_libs_txt}
            echo "${TARGET_OUT}/target/${ex_file}"
        fi
    done
}

pre_exclude_libs

before_reduce_size="`du ${ROOTFS} --max-depth=0 -k | awk {'print $1'}`"
ulibbefore_reduce_size="`du ${ROOTFS}/usr/lib --max-depth=0 -k | awk {'print $1'}`"
libbefore_reduce_size="`du ${ROOTFS}/lib --max-depth=0 -k | awk {'print $1'}`"
parse_needed_libs
after_reduce_size="`du ${ROOTFS} --max-depth=0 -k | awk {'print $1'}`"
ulibafter_reduce_size="`du ${ROOTFS}/usr/lib --max-depth=0 -k | awk {'print $1'}`"
libafter_reduce_size="`du ${ROOTFS}/lib --max-depth=0 -k | awk {'print $1'}`"
echo "Reduce *target* size [${before_reduce_size}] -> [${after_reduce_size}]"
echo "Reduce *usr/lib* size [${ulibbefore_reduce_size}] -> [${ulibafter_reduce_size}]"
echo "Reduce *lib* size [${libbefore_reduce_size}] -> [${libafter_reduce_size}]"

