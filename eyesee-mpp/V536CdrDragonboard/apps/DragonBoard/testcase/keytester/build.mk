TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
            keytester.c

TARGET_INC := $(TARGET_PATH)/../../../../include

TARGET_SHARED_LIB := \
    liblog \

TARGET_STATIC_LIB := \

TARGET_CPPFLAGS += -O2 -fPIC -Wall

TARGET_LDFLAGS += -lpthread
#    -lisp_dev -lisp_base -lisp_math -lisp_ae -lisp_af -lisp_afs \
#	-lisp_awb -lisp_md -lisp_iso -lisp_gtm -lisp_ini -liniparser \
#	-lpthread -lrt

ifeq ($(strip $(BOARD_TYPE)), PER1)
   TARGET_CFLAGS += -DFOX_PER1=1
endif

ifeq ($(strip $(BOARD_TYPE)), PRO)
   TARGET_CFLAGS += -DFOX_PRO=1
endif

TARGET_MODULE := keytester

include $(BUILD_BIN)
#########################################

