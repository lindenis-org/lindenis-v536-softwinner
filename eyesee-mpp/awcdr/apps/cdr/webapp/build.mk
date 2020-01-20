TARGET_PATH :=$(call my-dir)

INC_PATH += \
	$(TARGET_PATH) \
	$(TARGET_TOP)/custom_aw/include \
	$(TARGET_TOP)/custom_aw/apps/cdr/ \
	$(TARGET_TOP)/custom_aw/apps/cdr/source \
	$(TARGET_TOP)/custom_aw/apps/cdr/source/bll_presenter/remote \
    $(TARGET_TOP)/external/civetweb/include \
	$(TARGET_TOP)/system/public/include \

#########################################
include $(ENV_CLEAR)

SRC_TAGS := handler
TARGET_SRC := $(call all-cpp-files-under, $(SRC_TAGS))
TARGET_SRC += main.cpp

TARGET_INC := $(INC_PATH)

TARGET_CPPFLAGS +=

TARGET_LDFLAGS += \
    -lpthread

TARGET_STATIC_LIB := \
    libcivetweb \
    libwebserver \

TARGET_SHARED_LIB := \
    liblog \

TARGET_MODULE := webapp

include $(BUILD_BIN)
