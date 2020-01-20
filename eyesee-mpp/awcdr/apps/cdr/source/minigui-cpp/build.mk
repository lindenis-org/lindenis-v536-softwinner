#
# 1. Set the path and clear environment
#   TARGET_PATH := $(call my-dir)
#   include $(ENV_CLEAR)
#
# 2. Set the source files and headers files
#   TARGET_SRC := xxx_1.c xxx_2.c
#   TARGET_INc := xxx_1.h xxx_2.h
#
# 3. Set the output target
#   TARGET_MODULE := xxx
#
# 4. Include the main makefile
#   include $(BUILD_BIN)
#
# Before include the build makefile, you can set the compilaion
# flags, e.g. TARGET_ASFLAGS TARGET_CFLAGS TARGET_CPPFLAGS
#
TARGET_PATH :=$(call my-dir)
TOP_PATH := $(TARGET_PATH)

 # NOTE: 'runtime' must be build at first, because there are some
 # static data and the order of those data be initialized will affect
 # the behavior of linking grogram at runtime
SRC_TAGS += \
    source/runtime \
    source/data \
    source/debug \
    source/extra \
    source/memory \
    source/parser \
    source/resource \
    source/type \
    source/utils \
    source/widgets \
    source/window \

EXCLUDE_DIRS +=

SRC_TAGS := $(call filter-out, $(EXCLUDE_DIRS), $(SRC_TAGS))

#########################################
include $(ENV_CLEAR)

TARGET_SRC := source/application.cpp
TARGET_SRC += $(call all-cpp-files-under, $(SRC_TAGS))
TARGET_SRC += $(call all-c-files-under, $(SRC_TAGS))

TARGET_INC := \
    $(TARGET_PATH)/source \
    $(TARGET_TOP)/custom_aw/include \
    $(TARGET_TOP)/system/public/include

TARGET_STATIC_LIB := \
    libminigui_ths \
    libpng \
    libjpeg \
    libz

# we arrange this var use to set user build lib
TARGET_SHARED_LIB := \
    liblog

# and this var use to set system lib
TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl

TARGET_CPPFLAGS += \
    -fPIC \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \

TARGET_MODULE := libminigui-cpp
include $(BUILD_SHARED_LIB)

$(shell \
    cd $(TARGET_PATH)/source ; \
    find . -name \*.h | xargs -i{} cp --parents {} $(TARGET_TOP)/custom_aw/include/minigui-cpp ; \
    cd - > /dev/null \
)
