/**********************************************************
* Copyright (C), 2015, AllwinnerTech. Co., Ltd.  *
***********************************************************/
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

#include "device_qrencode.h"
#include "common/app_log.h"
#include "common/thread.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#undef LOG_TAG
#define LOG_TAG "Qrencode"

using namespace EyeseeLinux;

Qrencode::Qrencode(int feed_time)
    : feed_time_(feed_time)
    , timeout_(10)
{
}

Qrencode::~Qrencode()
{
    StopQrencode();
}
#if HAVE_PNG
void Qrencode::fillRow(unsigned char *row, int num, const unsigned char color[])
{
	int i;

	for(i = 0; i < num; i++) {
		memcpy(row, color, 4);
		row += 4;
	}
}
#endif
int Qrencode::writePNG(const QRcode *qrcode, const char *outfile, enum imageType type)
{
#ifdef QRCODE_SUPPORT
#if HAVE_PNG
	static FILE *fp; // avoid clobbering by setjmp.
	png_structp png_ptr;
	png_infop info_ptr;
	png_colorp palette = NULL;
	png_byte alpha_values[2];
	unsigned char *row, *p, *q;
	int x, y, xx, yy, bit;
	int realwidth;

	realwidth = (qrcode->width + margin * 2) * size;
	if(type == PNG_TYPE) {
		row = (unsigned char *)malloc(qrcode->width/*(realwidth + 7) / 8*/);
	} else if(type == PNG32_TYPE) {
		row = (unsigned char *)malloc(realwidth * 4);
	} else {
		fprintf(stderr, "Internal error.\n");
		return -1;
	}
	if(row == NULL) {
		fprintf(stderr, "Failed to allocate memory.\n");
		return -1;
	}

	if(outfile[0] == '-' && outfile[1] == '\0') {
		fp = stdout;
	} else {
		fp = fopen(outfile, "wb");
		if(fp == NULL) {
			fprintf(stderr, "Failed to create file: %s\n", outfile);
			perror(NULL);
			free(row);
			return -1;
		}
	}

	png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(png_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG writer.\n");
		free(row);
		return -1;
	}

	info_ptr = png_create_info_struct(png_ptr);
	if(info_ptr == NULL) {
		fprintf(stderr, "Failed to initialize PNG write.\n");
		free(row);
		return -1;
	}

	if(setjmp(png_jmpbuf(png_ptr))) {
		png_destroy_write_struct(&png_ptr, &info_ptr);
		fprintf(stderr, "Failed to write PNG image.\n");
		free(row);
		return -1;
	}

	if(type == PNG_TYPE) {
		palette = (png_colorp) malloc(sizeof(png_color) * 2);
		if(palette == NULL) {
			fprintf(stderr, "Failed to allocate memory.\n");
			png_destroy_write_struct(&png_ptr, &info_ptr);
			free(row);
			return -1;
		}
		palette[0].red   = fg_color[0];
		palette[0].green = fg_color[1];
		palette[0].blue  = fg_color[2];
		palette[1].red   = bg_color[0];
		palette[1].green = bg_color[1];
		palette[1].blue  = bg_color[2];
		alpha_values[0] = fg_color[3];
		alpha_values[1] = bg_color[3];
		png_set_PLTE(png_ptr, info_ptr, palette, 2);
		png_set_tRNS(png_ptr, info_ptr, alpha_values, 2, NULL);
	}

	png_init_io(png_ptr, fp);
	if(type == PNG_TYPE) {
		png_set_IHDR(png_ptr, info_ptr,
				realwidth, realwidth,
				1,
				PNG_COLOR_TYPE_PALETTE,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
	} else {
		png_set_IHDR(png_ptr, info_ptr,
				realwidth, realwidth,
				8,
				PNG_COLOR_TYPE_RGB_ALPHA,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);
	}
	png_set_pHYs(png_ptr, info_ptr,
			dpi * INCHES_PER_METER,
			dpi * INCHES_PER_METER,
			PNG_RESOLUTION_METER);
	png_write_info(png_ptr, info_ptr);

	if(type == PNG_TYPE) {
	/* top margin */
		memset(row, 0xff, qrcode->width/*(realwidth + 7) / 8*/);
		for(y = 0; y < margin * size; y++) {
			png_write_row(png_ptr, row);
		}

		/* data */
		p = qrcode->data;
		for(y = 0; y < qrcode->width; y++) {
			memset(row, 0xff, qrcode->width/*(realwidth + 7) / 8*/);
			q = row;
			q += margin * size / 8;
			bit = 7 - (margin * size % 8);
			for(x = 0; x < qrcode->width; x++) {
				for(xx = 0; xx < size; xx++) {
					*q ^= (*p & 1) << bit;
					bit--;
					if(bit < 0) {
						q++;
						bit = 7;
					}
				}
				p++;
			}
			for(yy = 0; yy < size; yy++) {
				png_write_row(png_ptr, row);
			}
		}
		/* bottom margin */
		memset(row, 0xff, qrcode->width/*(realwidth + 7) / 8*/);
		for(y = 0; y < margin * size; y++) {
			png_write_row(png_ptr, row);
		}
	} else {
	/* top margin */
		fillRow(row, realwidth, bg_color);
		for(y = 0; y < margin * size; y++) {
			png_write_row(png_ptr, row);
		}

		/* data */
		p = qrcode->data;
		for(y = 0; y < qrcode->width; y++) {
			fillRow(row, realwidth, bg_color);
			for(x = 0; x < qrcode->width; x++) {
				for(xx = 0; xx < size; xx++) {
					if(*p & 1) {
						memcpy(&row[((margin + x) * size + xx) * 4], fg_color, 4);
					}
				}
				p++;
			}
			for(yy = 0; yy < size; yy++) {
				png_write_row(png_ptr, row);
			}
		}
		/* bottom margin */
		fillRow(row, realwidth, bg_color);
		for(y = 0; y < margin * size; y++) {
			png_write_row(png_ptr, row);
		}
	}

	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);

	fclose(fp);
	free(row);
	free(palette);

	return 0;
#else
	//fputs("PNG output is disabled at compile time. No output generated.\n", stderr);
	int i,j;
	printf("\n@@@ version %d, width %d  @@@\n", qrcode->version, qrcode->width);
	for(i=0; i<qrcode->width; i++)
	{
		for(j=0; j<qrcode->width; j++)
		{
			if(qrcode->data[i*qrcode->width+j]&0x01) 
				printf("#");
			else
				printf(" ");
		}
		printf("\r\n");
	}
	printf("\r\n");
	return 0;
#endif
#else
	return 0;
#endif
}

QRcode *Qrencode::encode(const unsigned char *intext, int length)
{
#ifdef QRCODE_SUPPORT
	QRcode *code;

	code = QRcode_encodeString((char *)intext, version, level, hint, casesensitive);

	return code;
#else
	return NULL;
#endif
}

void Qrencode::qrencode(const unsigned char *intext, int length, const char *outfile)
{
#ifdef QRCODE_SUPPORT
	QRcode *qrcode;

	qrcode = encode(intext, length);
	if(qrcode == NULL) {
		if(errno == ERANGE) {
			fprintf(stderr, "Failed to encode the input data: Input data too large\n");
		} else {
			perror("Failed to encode the input data");
		}
		return ;
	}

	if(verbose) {
		fprintf(stderr, "File: %s, Version: %d\n", (outfile!=NULL)?outfile:"(stdout)", qrcode->version);
	}

	switch(image_type) {	// only support PNG
		case PNG_TYPE:
		case PNG32_TYPE:
			writePNG(qrcode, outfile, image_type);
			break;
		default:
			fprintf(stderr, "Unknown image type.\n");
			return ;
	}

	QRcode_free(qrcode);
    return;
#else
    return;
#endif
}

int8_t Qrencode::RunQrencode(std::string imei, std::string sim)
{
#ifdef QRCODE_SUPPORT
	//code = imei.c_str() + sim.c_str();
	code = imei + "_" + sim;
    ThreadCreate(&rencode_tid_, NULL, Qrencode::QrencodeThread, this);

    return 0;
#else
    return 0;
#endif
}

int8_t Qrencode::StopQrencode()
{
    return 0;
}

void Qrencode::QrencodeCreat(const unsigned char *src, int length)
{
#ifdef QRCODE_SUPPORT
    version = 8;
    margin = 4;
    level = QR_ECLEVEL_H;
    const char code_path[] = "/data/UberBindimage.png";

    this->qrencode(src, length, code_path);

    return;
#else
    return;
#endif
}

void *Qrencode::QrencodeThread(void *context)
{
#ifdef QRCODE_SUPPORT
    Qrencode *self = reinterpret_cast<Qrencode*>(context);
    prctl(PR_SET_NAME, "QrencodeThread", 0, 0, 0);
	version = 6;
	margin = 4;
	level = QR_ECLEVEL_H;
	const char code_path[] = "/data/BindImage.png";
	//self->qrencode((const unsigned char *)self->code.c_str(), sizeof(self->code.c_str()), code_path);
	self->qrencode((const unsigned char *)self->code.c_str(), self->code.length(), code_path);
	//
	version = 11;
	const unsigned char wifiAppQR_string[] = "123456789";
	const char wifiAppQR_path[] = "/data/WifiAppQR.png";
	self->qrencode(wifiAppQR_string, sizeof(wifiAppQR_string), wifiAppQR_path);
    return NULL;
#else
    return NULL;
#endif
}
