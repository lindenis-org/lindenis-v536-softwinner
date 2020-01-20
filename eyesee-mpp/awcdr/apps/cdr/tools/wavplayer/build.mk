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

TARGET_PATH:= $(call my-dir)

########################################
include $(ENV_CLEAR)

TARGET_SRC := sys_pre.c alsa_interface.c bufferMessager.c pcmBufferManager.c

TARGET_INC := \
    $(TARGET_TOP)/middleware/include/utils \
    $(TARGET_TOP)/middleware/include/media \
    $(TARGET_TOP)/middleware/include \
    $(TARGET_TOP)/middleware/media/include/utils \
    $(TARGET_TOP)/middleware/media/include/component \

TARGET_SHARED_LIB := \
    librt \
    libpthread \
    libasound \
    liblog \
    libdl \

TARGET_STATIC_LIB := \
    libaw_mpp \
    libmedia_utils \


#TARGET_CPPFLAGS += -fPIC -Wall -Wno-unused-but-set-variable
TARGET_CFLAGS += -fPIC -Wall -Wno-unused-but-set-variable

#TARGET_LDFLAGS += -static

TARGET_MODULE := wavplayer

include $(BUILD_BIN)

#########################################
