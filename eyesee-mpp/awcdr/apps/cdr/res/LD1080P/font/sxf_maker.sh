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
    w=26
fi

#default h
if [ "$h" == "" ]; then
    h=26
fi

grep -oh ">.*<" ../lang/zh-CN.xml | grep -o "[^<|^>]" | sort > ../lang/zh-CN.symbol
grep -oh ">.*<" ../lang/zh-DE.xml | grep -o "[^<|^>]" | sort > ../lang/zh-DE.symbol
grep -oh ">.*<" ../lang/zh-EN.xml | grep -o "[^<|^>]" | sort > ../lang/zh-EN.symbol
grep -oh ">.*<" ../lang/zh-ES.xml | grep -o "[^<|^>]" | sort > ../lang/zh-ES.symbol
grep -oh ">.*<" ../lang/zh-FR.xml | grep -o "[^<|^>]" | sort > ../lang/zh-FR.symbol
grep -oh ">.*<" ../lang/zh-IT.xml | grep -o "[^<|^>]" | sort > ../lang/zh-IT.symbol
grep -oh ">.*<" ../lang/zh-JA.xml | grep -o "[^<|^>]" | sort > ../lang/zh-JA.symbol
grep -oh ">.*<" ../lang/zh-KO.xml | grep -o "[^<|^>]" | sort > ../lang/zh-KO.symbol
grep -oh ">.*<" ../lang/zh-PT.xml | grep -o "[^<|^>]" | sort > ../lang/zh-PT.symbol
grep -oh ">.*<" ../lang/zh-RU.xml | grep -o "[^<|^>]" | sort > ../lang/zh-RU.symbol
grep -oh ">.*<" ../lang/zh-TW.xml | grep -o "[^<|^>]" | sort > ../lang/zh-TW.symbol
grep -oh ">.*<" ../lang/zh-TI.xml | grep -o "[^<|^>]" | sort > ../lang/zh-TI.symbol


# generate font sxf_${width}x${height}.bin
# usage
# ./sxf_make font_file width height stdout>null stderr>null
#
./sxf_make $font_file ../lang/ $w $h

rm sxf_${font_file%.*}_"$w"x"$h".7z ../lang/*.symbol

# use 7zip to compress font file
# 7zip version: p7zip_9.20.1_src_all.tar.bz2
./7za a sxf_${font_file%.*}_"$w"x"$h".7z sxf_"$w"x"$h".bin


#delete the font file
rm sxf_"$w"x"$h".bin
