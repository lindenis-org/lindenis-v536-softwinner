#
# 1. Set the path and clear environment
# 	TARGET_PATH := $(call my-dir)
# 	include $(ENV_CLEAR)
#
# 2. Set the source files and headers files
#	TARGET_SRC := xxx_1.c xxx_2.c
#	TARGET_INc := xxx_1.h xxx_2.h
#
# 3. Set the output target
#	TARGET_MODULE := xxx
#
# 4. Include the main makefile
#	include $(BUILD_BIN)
#
# Before include the build makefile, you can set the compilaion
# flags, e.g. TARGET_ASFLAGS TARGET_CFLAGS TARGET_CPPFLAGS
#

TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)
include $(TARGET_TOP)/middleware/config/mpp_config.mk

TARGET_CPPFLAGS += $(CEDARX_EXT_CFLAGS)
TARGET_CFLAGS += $(CEDARX_EXT_CFLAGS)

TARGET_SRC := dragonboard.cpp \

TARGET_INC := \
    $(TARGET_TOP)/system/include \
    $(TARGET_TOP)/middleware/media/vi_api \
    $(TARGET_TOP)/middleware/include \
    $(TARGET_TOP)/middleware/include/utils \
    $(TARGET_TOP)/middleware/include/media \
    $(TARGET_TOP)/middleware/media/include/component \
	$(TARGET_TOP)/middleware/media/LIBRARY/libisp/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/isp_tuning \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_ai_common \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_eve_common \
    $(TARGET_TOP)/middleware/media/LIBRARY/libaiMOD/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libVLPR/include \
	$(TARGET_TOP)/middleware/media/LIBRARY/libeveface/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_stream \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_FsWriter \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarc/include \
	$(TARGET_TOP)/middleware/media/LIBRARY/include_muxer \
	$(TARGET_TOP)/middleware/media/LIBRARY/libISE \
    $(TARGET_TOP)/framework/include \
    $(TARGET_TOP)/framework/include/utils \
    $(TARGET_TOP)/custom_aw/include \
    $(TARGET_TOP)/custom_aw/include/V5SDVTP \
    $(TARGET_TOP)/framework/include/media/camera \
    $(TARGET_TOP)/framework/include/media/ise \
    $(TARGET_TOP)/system/public/include \
    $(TARGET_TOP)/system/public/include/lua/ \
    $(TARGET_TOP)/framework/include/media/recorder \
    $(TARGET_PATH)/include \

TARGET_STATIC_LIB := \
    libaw_mpp \
    libmedia_utils \
    libcedarx_aencoder \
    libaacenc \
    libadpcmenc \
    libg711enc \
    libg726enc \
    libmp3enc \
    libadecoder \
    libcedarxdemuxer \
    libAwNosc \
    libvdecoder \
    libvideoengine \
    libawh264 \
    libawh265 \
    libawmjpegplus \
    libvencoder \
    libMemAdapter \
    libVE \
    libcdc_base \
    libISP \
    libisp_ae \
    libisp_af \
    libisp_afs \
    libisp_awb \
    libisp_base \
    libisp_gtm \
    libisp_iso \
    libisp_math \
    libisp_md \
    libisp_pltm \
    libisp_rolloff \
    libmatrix \
    libiniparser \
    libisp_ini \
    libisp_dev \
    libmuxers \
    libmp4_muxer \
    libraw_muxer \
    libmpeg2ts_muxer \
    libaac_muxer \
    libmp3_muxer \
    libffavutil \
    libFsWriter \
    libcedarxstream \
    libion \
    libhwdisplay \
    libwifi_sta \
    libwifi_ap \
    libwpa_ctl \
    librgb_ctrl \
    liblz4 \
    libcutils \

# we arrange this var use to set user build lib
TARGET_SHARED_LIB := \
    libcdx_base \
    libcdx_parser \
    libcdx_stream \
    libcdx_common \
    libasound \
    liblog \
    # libcedarxrender \
    # libmpp_vi \
    # libmpp_isp \
    # libmpp_vo \
    # libmpp_component \
    # libmedia_mpp \
    # libISP \

# and this var use to set system lib
TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl

#######################################################################
ifeq ($(MPPCFG_USE_KFC),Y)
    TARGET_STATIC_LIB += lib_hal
endif


TARGET_STATIC_LIB += \
    libminigui_ths \
    libpng \
    libjpeg \
    libz \
	libts

TARGET_CPPFLAGS += -fPIC -Wall -Wno-unused-but-set-variable
TARGET_CFLAGS += -fPIC -Wall -Wno-unused-but-set-variable

TARGET_CFLAGS += \
	-fPIC

ifeq ($(strip $(BOARD_TYPE)), PER1)
	TARGET_CPPFLAGS += -DFOX_PER1=1
endif


ifeq ($(strip $(BOARD_TYPE)), PRO)
   TARGET_CPPFLAGS += -DFOX_PRO=1
endif



TARGET_MODULE := dragonboard
include $(BUILD_BIN)
#include $(BUILD_SHARED_LIB)

