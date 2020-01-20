#!/bin/bash

font_file=arialuni.ttf

while getopts fw:h:t: option
do
    case "$option" in
        f)
            force_build="y";;
        w)
            w=$OPTARG;;
        h)
            h=$OPTARG;;
        t)
            ttf_file=$OPTARG;;
        i)
            all_info="y";;
        \?)
            echo "Usage: args [-f] [-w width] [-h height]"
            echo "-f force to build"
            echo "-w font width"
            echo "-h font height"
            echo "-t font file"
            exit 1;;
    esac
done

file=../lang/zh-CN.xml
md5_file=../lang/.md5
fmd5()
{
    find $file -type f | xargs md5sum | cut -d ' ' -f 1 > $md5_file
}

#-f Forced to build
if [ "$force_build" != "y" ]; then
    if [ ! -f $md5_file ]; then
        fmd5
    else
        read old_md5 < $md5_file
    fi
    new_md5=`find $file -type f | xargs md5sum | cut -d ' ' -f 1`
    if [ "$old_md5" == "$new_md5" ]; then
        need_rebuild=0
    else
        echo "lang is changed"
        need_rebuild=1
    fi
    echo $new_md5 > $md5_file

    if [ $need_rebuild == 0 ]; then
        exit
    fi
fi

#default w
if [ "$w" == "" ]; then
    w=30
fi

#default h
if [ "$h" == "" ]; then
    h=30
fi

grep -oh ">.*<" ../lang/zh-CN.xml | grep -o "[^<|^>]" | sort | uniq > ../lang/zh-CN.symbol


# generate font sxf_${width}x${height}.bin
# usage
# ./sxf_make font_file width height stdout>null stderr>null
#
./sxf_make $font_file ../lang/ $w $h

rm sxf_${font_file%.*}.7z ../lang/*.symbol

# use 7zip to compress font file
# 7zip version: p7zip_9.20.1_src_all.tar.bz2
./7za a sxf_${font_file%.*}.7z sxf_"$w"x"$h".bin


#delete the font file
rm sxf_"$w"x"$h".bin
