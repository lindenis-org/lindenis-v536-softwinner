TARGET_PATH :=$(call my-dir)

#########################################
include $(ENV_CLEAR)

TARGET_SRC := \
            wifitester_v5_station.c
            
TARGET_INC := $(TARGET_PATH)/../../../../include \
	      $(TARGET_TOP)/system/include 

TARGET_SHARED_LIB := \
    liblog \


TARGET_STATIC_LIB := \
    libwifi_ap \
    libwifi_sta \
    libwpa_ctl

TARGET_CPPFLAGS += -O2 -fPIC -Wall

TARGET_LDFLAGS += -lpthread
#    -lisp_dev -lisp_base -lisp_math -lisp_ae -lisp_af -lisp_afs \
#	-lisp_awb -lisp_md -lisp_iso -lisp_gtm -lisp_ini -liniparser \
#	-lpthread -lrt

TARGET_MODULE := wifitester

include $(BUILD_BIN)
#########################################


$(call copy-files-under, \
        $(TARGET_PATH)/wpa_supplicant, \
        $(TARGET_OUT)/target/usr/sbin/ \
)


$(call copy-files-under, \
        $(TARGET_PATH)/wpa_passphrase, \
        $(TARGET_OUT)/target/usr/sbin/ \
)


$(call copy-files-under, \
        $(TARGET_PATH)/wpa_cli, \
        $(TARGET_OUT)/target/usr/sbin/ \
)



