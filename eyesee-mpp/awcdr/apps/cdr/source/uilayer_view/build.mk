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

SRC_TAGS := \
	gui/minigui \

EXCLUDE_DIRS :=

SRC_TAGS := $(call filter-out, $(EXCLUDE_DIRS), $(SRC_TAGS))

#########################################
include $(ENV_CLEAR)

TARGET_SRC := $(call all-cpp-files-under, $(SRC_TAGS))
TARGET_SRC += $(call all-c-files-under, $(SRC_TAGS))
TARGET_SRC += \
	../main.cpp

TARGET_INC := \
	$(TARGET_PATH)/gui/minigui \
	$(TARGET_PATH)/..\
	$(TARGET_PATH)/../../../../include

TARGET_SHARED_LIB := liblog
TARGET_CPPFLAGS +=
TARGET_LDFLAGS += \
	-lpthread \
	-lminigui_ths \
	-lpng
TARGET_MODULE := ui2.0
include $(BUILD_BIN)
