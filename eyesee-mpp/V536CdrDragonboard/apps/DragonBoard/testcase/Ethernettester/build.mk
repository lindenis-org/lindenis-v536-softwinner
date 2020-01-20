TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)
include $(TARGET_TOP)/middleware/config/mpp_config.mk

TARGET_CPPFLAGS += $(CEDARX_EXT_CFLAGS)
TARGET_CFLAGS += $(CEDARX_EXT_CFLAGS)
TARGET_SRC := \
            ethernettest.c
            
TARGET_INC := $(TARGET_PATH)/../../../../include

TARGET_SHARED_LIB := \
    librt \
    libpthread \
    liblog \
    libcustomaw_media_utils \
    libhwdisplay \


TARGET_STATIC_LIB := \
    libcamera \

#    libisp_dev \
#    libisp_base \
#    libisp_math \
#    libisp_ae \
#    libisp_af \
#    libisp_afs \
#    libisp_awb \
#    libisp_md \
#    libisp_iso \
#    libisp_gtm \
#    libisp_ini \
#    libiniparser 

TARGET_CPPFLAGS += -O2 -fPIC -Wall

TARGET_LDFLAGS += 
#    -lisp_dev -lisp_base -lisp_math -lisp_ae -lisp_af -lisp_afs \
#	-lisp_awb -lisp_md -lisp_iso -lisp_gtm -lisp_ini -liniparser \
#	-lpthread -lrt

TARGET_MODULE := ethernettest

include $(BUILD_BIN)
#include $(BUILD_SHARED_LIB)
#########################################

