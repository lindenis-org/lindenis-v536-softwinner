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
include $(ENV_CLEAR)

$(call copy-one-file, \
	$(TARGET_PATH)/libcrypto_aw.so, \
	$(TARGET_OUT)/target/usr/lib/libcrypto_aw.so \
)

#########################################
#include $(ENV_CLEAR)
#TARGET_MODULE := libcrypto_aw
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)
 
#########################################
#include $(ENV_CLEAR)
#TARGET_MODULE := lib_ise_bi
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)

#########################################
#include $(ENV_CLEAR)
#TARGET_MODULE := libevePatternTest
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)

#########################################

#include $(ENV_CLEAR)
#TARGET_MODULE := libcve_filter
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)

#########################################
#include $(ENV_CLEAR)
#TARGET_MODULE := libCVEMiddleInterface
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)

#########################################

#include $(ENV_CLEAR)
#TARGET_MODULE := libcve_key_test
#TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
#include $(BUILD_MULTI_PREBUILT)

#########################################


