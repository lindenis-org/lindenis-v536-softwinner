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

TARGET_SRC := \
	utils.cpp \
	xml_parser.cpp \
    OtaUpdate.cpp \
    OtaMain.cpp \
    imgdecode.c \
    sprite_card.c \
    sprite_main.c \
    update4GModule.cpp \
    mtd_opration.c

TARGET_INC += \
    $(TARGET_PATH) \
    $(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT) \
	$(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT)/minigui \
    ./include

TARGET_STATIC_LIB := \
    libminigui_ths \
    libpng \
    libjpeg \
    libz \

TARGET_SHARED_LIB := \
    liblog \
    libts

TARGET_CFLAGS += \
    -fPIC

TARGET_LDFLAGS += \
    -lpthread \

TARGET_MODULE := ota_update

include $(BUILD_BIN)
