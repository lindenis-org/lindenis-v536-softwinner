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
TARGET_MODULE := libminigui_ths.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################
# include $(ENV_CLEAR)
# TARGET_MODULE := libcrypto
# TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
# include $(BUILD_MULTI_PREBUILT)

#########################################
 # include $(ENV_CLEAR)
 # TARGET_MODULE := libaf_alg
# # TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/lib$(AF_ALG_V)*.so)
 # TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
 # include $(BUILD_MULTI_PREBUILT)

#########################################
# include $(ENV_CLEAR)
# TARGET_MODULE := libssl.a
# TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
# include $(BUILD_MULTI_PREBUILT)
#
#########################################
# tutk
include $(ENV_CLEAR)
TARGET_MODULE := libAVAPIs.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/tutk/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

include $(ENV_CLEAR)
TARGET_MODULE := libIOTCAPIs.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/tutk/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################
# include $(ENV_CLEAR)
# TARGET_MODULE := libTinyServer
# TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
# include $(BUILD_MULTI_PREBUILT)

#########################################
# include $(ENV_CLEAR)
# TARGET_MODULE := libOnvif
# TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_MODULE)*)
# include $(BUILD_MULTI_PREBUILT)

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := edog.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libqrencode.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libjsoncpp.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libpaho-embed-mqtt3c.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libpaho-embed-mqtt3cc.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libdd_serv.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################

#########################################

include $(ENV_CLEAR)
TARGET_MODULE := libcurl.a
TARGET_PREBUILT_LIBS := $(wildcard $(TARGET_PATH)/$(TARGET_PRODUCT)/$(TARGET_MODULE)*)
include $(BUILD_MULTI_PREBUILT)

#########################################
