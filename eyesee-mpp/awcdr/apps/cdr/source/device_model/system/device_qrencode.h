/******************************************************************************
  Copyright (C), 2001-2016, Allwinner Tech. Co., Ltd.
 ******************************************************************************/
/**
 * @file device_qrencode.h
 * @brief 二维码生成
 *
 *
 * @author id:007
 * @date 2018-02=5-14
 *
 * @verbatim
    History:
   @endverbatim
 */

#pragma once

#include <pthread.h>
#include <stdint.h>
#include <png.h>
#include <qrencode/qrencode.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <stdio.h>
#include <string>
#include <vector>


#define HAVE_PNG 1
#define INCHES_PER_METER (100.0/2.54)

static int casesensitive = 1;
static int eightbit = 0;
static int version = 6;
static int size = 3;
static int margin = -1;
static int dpi = 72;
static int structured = 0;
static int rle = 0;
static int svg_path = 0;
static int micro = 0;
static QRecLevel level = QR_ECLEVEL_L;
static QRencodeMode hint = QR_MODE_8;
static unsigned char fg_color[4] = {0, 0, 0, 255};
static unsigned char bg_color[4] = {255, 255, 255, 255};

static int verbose = 0;
enum imageType {
	PNG_TYPE,
	PNG32_TYPE,
	EPS_TYPE,
	SVG_TYPE,
	XPM_TYPE,
	ANSI_TYPE,
	ANSI256_TYPE,
	ASCII_TYPE,
	ASCIIi_TYPE,
	UTF8_TYPE,
	ANSIUTF8_TYPE,
	UTF8i_TYPE,
	ANSIUTF8i_TYPE
};

static enum imageType image_type = PNG_TYPE;

static const struct option options[] = {
	{"help"         , no_argument      , NULL, 'h'},
	{"output"       , required_argument, NULL, 'o'},
	{"read-from"    , required_argument, NULL, 'r'},
	{"level"        , required_argument, NULL, 'l'},
	{"size"         , required_argument, NULL, 's'},
	{"symversion"   , required_argument, NULL, 'v'},
	{"margin"       , required_argument, NULL, 'm'},
	{"dpi"          , required_argument, NULL, 'd'},
	{"type"         , required_argument, NULL, 't'},
	{"structured"   , no_argument      , NULL, 'S'},
	{"kanji"        , no_argument      , NULL, 'k'},
	{"casesensitive", no_argument      , NULL, 'c'},
	{"ignorecase"   , no_argument      , NULL, 'i'},
	{"8bit"         , no_argument      , NULL, '8'},
	{"rle"          , no_argument      , &rle,   1},
	{"svg-path"     , no_argument      , &svg_path, 1},
	{"micro"        , no_argument      , NULL, 'M'},
	{"foreground"   , required_argument, NULL, 'f'},
	{"background"   , required_argument, NULL, 'b'},
	{"version"      , no_argument      , NULL, 'V'},
	{"verbose"      , no_argument      , &verbose, 1},
	{NULL, 0, NULL, 0}
};

static char *optstring = "ho:r:l:s:v:m:d:t:Skci8MV";

//using namespace std;

namespace EyeseeLinux {
class Qrencode
{
    public:
        Qrencode(int feed_time = 3);

        ~Qrencode();

        int8_t RunQrencode(std::string imei, std::string sim);

        int8_t StopQrencode();
		void fillRow(unsigned char *row, int num, const unsigned char color[]);
		
		int writePNG(const QRcode *qrcode, const char *outfile, enum imageType type);
		
		QRcode *encode(const unsigned char *intext, int length);
	
        void QrencodeCreat(const unsigned char *src, int length);
        //void QrencodeCreat(std::string src);

		void qrencode(const unsigned char *intext, int length, const char *outfile);
		
        static void *QrencodeThread(void *context);
		
    private:
	 std::string code;
        int feed_time_;
        int timeout_; // min:1s, max:16s
        pthread_t rencode_tid_;
        int8_t FeedQrencode();
};

}
