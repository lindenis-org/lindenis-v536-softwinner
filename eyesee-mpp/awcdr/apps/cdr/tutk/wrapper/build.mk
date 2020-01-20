TARGET_PATH :=$(call my-dir)

SRC_FILES += \
	av_ctrl.cpp \
	av_server.cpp \
	tutk_wrapper.cpp \
	tutk_util.cpp

INC_PATH += \
	$(TARGET_PATH) \
	$(TARGET_TOP)/custom_aw/include \
	$(TARGET_TOP)/framework/include \
	$(TARGET_TOP)/custom_aw/apps/cdr/source \
	$(TARGET_TOP)/custom_aw/apps/cdr/source/bll_presenter/remote \

COMM_FLAGS = -DDEBUG_LOG

#########################################
include $(ENV_CLEAR)

TARGET_SRC := $(SRC_FILES)
TARGET_INC := $(INC_PATH)

TARGET_CPPFLAGS += -fPIC $(COMM_FLAGS)

TARGET_STATIC_LIB := \
	libAVAPIs \
	libIOTCAPIs \

TARGET_MODULE := libtutk

include $(BUILD_STATIC_LIB)

#########################################
# include $(ENV_CLEAR)

# TARGET_SRC := $(SRC_FILES)
# TARGET_INC := $(INC_PATH)

# TARGET_CPPFLAGS += -fPIC $(COMM_FLAGS)

# TARGET_SHARED_LIB := \
	# libAVAPIs \
	# libIOTCAPIs \

# TARGET_MODULE := libtutk

# include $(BUILD_SHARED_LIB)

# #######################################

# include $(ENV_CLEAR)

# TARGET_SRC := main.cpp
# TARGET_INC := $(TARGET_PATH)/include

# TARGET_CPPFLAGU += -v
# TARGET_LDFLAGS += -ldemo -static

# TARGET_MODULE := main

# include $(BUILD_BIN)
