TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
            tftester.c
            
TARGET_INC := $(TARGET_PATH)/../../../../include

TARGET_SHARED_LIB := \
    liblog \

TARGET_STATIC_LIB := \

TARGET_CPPFLAGS += -O2 -fPIC -Wall

TARGET_LDFLAGS += -lpthread
#    -lisp_dev -lisp_base -lisp_math -lisp_ae -lisp_af -lisp_afs \
#	-lisp_awb -lisp_md -lisp_iso -lisp_gtm -lisp_ini -liniparser \
#	-lpthread -lrt

TARGET_MODULE := tftester

include $(BUILD_BIN)
#########################################

