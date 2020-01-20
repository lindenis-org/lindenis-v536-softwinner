TARGET_PATH :=$(call my-dir)

SRC_FILES += \
	webserver.cpp

INC_PATH += \
	$(TARGET_PATH) \
	$(TARGET_TOP)/custom_aw/include \
	$(TARGET_TOP)/custom_aw/apps/cdr/source \
	$(TARGET_TOP)/custom_aw/apps/cdr/source/bll_presenter/remote \
    $(TARGET_TOP)/external/civetweb/include \
	$(TARGET_TOP)/system/public/include \

COMM_FLAGS = -DDEBUG_LOG

#########################################
include $(ENV_CLEAR)

TARGET_SRC := $(SRC_FILES)
TARGET_INC := $(INC_PATH)

TARGET_CPPFLAGS += -fPIC $(COMM_FLAGS)

TARGET_STATIC_LIB := \
	libglog \
	liblog \
    libcivetweb \

TARGET_MODULE := libwebserver

include $(BUILD_STATIC_LIB)

#########################################
# include $(ENV_CLEAR)

# TARGET_SRC := $(SRC_FILES)
# TARGET_INC := $(INC_PATH)

# TARGET_CPPFLAGS += -fPIC $(COMM_FLAGS)

# TARGET_SHARED_LIB := \
    # liblog \
    # libcivetweb \

# TARGET_MODULE := libwebserver

# include $(BUILD_SHARED_LIB)
