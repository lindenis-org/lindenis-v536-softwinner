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
include $(call my-dir)/app_config.mk
TARGET_PATH :=$(call my-dir)
TOP_PATH := $(TARGET_PATH)

SRC_TAGS := \
    source/bll_presenter/remote \
    source/common \
    source/device_model

EXCLUDE_DIRS +=

SRC_TAGS := $(call filter-out, $(EXCLUDE_DIRS), $(SRC_TAGS))

#########################################
include $(ENV_CLEAR)
include $(TARGET_TOP)/middleware/config/mpp_config.mk

TARGET_SRC := $(call all-cpp-files-under, $(SRC_TAGS))
TARGET_SRC += $(call all-c-files-under, $(SRC_TAGS))
TARGET_SRC += source/main.cpp

TARGET_INC := \
    $(TARGET_PATH) \
    $(TARGET_PATH)/source \
    $(TARGET_PATH)/tutk/wrapper \
    $(TARGET_PATH)/source/bll_presenter/remote \
    $(TARGET_PATH)/source/bll_presenter/remote/interface \
    $(TARGET_PATH)/source/uilayer_view/gui/minigui \
    $(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT)/rtsp \
    $(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT) \
    $(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT)/dd_serv \
    $(TARGET_TOP)/custom_aw/include/$(TARGET_PRODUCT)/minigui \
    $(TARGET_TOP)/framework/include \
    $(TARGET_TOP)/framework/include/media \
    $(TARGET_TOP)/framework/include/utils \
    $(TARGET_TOP)/framework/include/media/bdii \
    $(TARGET_TOP)/framework/include/media/camera \
    $(TARGET_TOP)/framework/include/media/recorder \
	$(TARGET_TOP)/framework/include/media/thumbretriever \
	$(TARGET_TOP)/framework/include/media/videoresizer \
    $(TARGET_TOP)/framework/include/media/player \
    $(TARGET_TOP)/framework/include/media/ise \
    $(TARGET_TOP)/middleware/include \
    $(TARGET_TOP)/middleware/include/utils \
    $(TARGET_TOP)/middleware/include/media \
    $(TARGET_TOP)/middleware/media/include/utils \
    $(TARGET_TOP)/middleware/media/include/component \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libISE \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
    $(TARGET_TOP)/middleware/media/LIBRARY/libisp/isp_tuning \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_ai_common \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_eve_common \
    $(TARGET_TOP)/middleware/media/LIBRARY/libaiMOD/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_stream \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_FsWriter \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarc/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libeveface/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/libVLPR/include \
    $(TARGET_TOP)/middleware/media/LIBRARY/include_muxer \
    $(TARGET_TOP)/middleware/media/LIBRARY/libcedarx/libcore/common/iniparser \
    $(TARGET_TOP)/middleware/sample/configfileparser \
    $(TARGET_TOP)/system/public/bt/demo/socket_test/include \
    $(TARGET_TOP)/system/public/wifi \
    $(TARGET_TOP)/system/public/rgb_ctrl \
    $(TARGET_TOP)/system/public/smartlink \
    $(TARGET_TOP)/system/public/include \
    $(TARGET_PATH)/include \
    $(TARGET_PATH)/uber \
    $(TARGET_TOP)/external/smartDriving/include
#	$(TARGET_PATH)/dd_serv \

TARGET_STATIC_LIB := \
    libcamera \
    librecorder \
	libthumbretriever \
	libvideoresizer \
    libplayer \
    libcustomaw_media_utils \
    libaw_mpp \
    libmedia_utils \
    libcedarx_aencoder \
    libaacenc \
    libadpcmenc \
    libg711enc \
    libg726enc \
    libmp3enc \
    libadecoder \
    libcedarxdemuxer \
    libAwNosc \
    libvdecoder \
    libvideoengine \
    libawh264 \
    libawh265 \
    libawmjpegplus \
    libvencoder \
    libMemAdapter \
    libVE \
    libcdc_base \
    libISP \
    libisp_ae \
    libisp_af \
    libisp_afs \
    libisp_awb \
    libisp_base \
    libisp_gtm \
    libisp_iso \
    libisp_math \
    libisp_md \
    libisp_pltm \
    libisp_rolloff \
    libiniparser \
    libisp_ini \
    libisp_dev \
    libmuxers \
    libmp4_muxer \
    libraw_muxer \
    libmpeg2ts_muxer \
    libaac_muxer \
    libmp3_muxer \
    libffavutil \
    libFsWriter \
    libcedarxstream \
    libion \
    libhwdisplay \
    libwifi_sta \
    libwifi_ap \
    libwpa_ctl \
    librgb_ctrl \
    liblz4 \
    libluaconfig \
    liblua \
    libSQLiteCpp \
    libsqlite3 \
    libcutils \
    libvencoder \
    libvenc_codec \
    libvenc_base 
# we arrange this var use to set user build lib
TARGET_SHARED_LIB := \
    libcdx_base \
    libcdx_parser \
    libcdx_stream \
    libcdx_common \
    libasound \
    liblog \
    libssl \
    libcrypto \
    libsample_confparser 
    # libcedarxrender \
    # libmpp_vi \
    # libmpp_isp \
    # libmpp_vo \
    # libmpp_component \
    # libmedia_mpp \
    # libISP \

# and this var use to set system lib
TARGET_LDFLAGS += \
    -lpthread \
    -lrt \
    -ldl

TARGET_CPPFLAGS += \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-switch \
    -Wno-pointer-arith \
    -fexceptions \
    -static-libstdc++ \
    -DHAS_BDROID_BUILDCFG -DLINUX_NATIVE -DANDROID_USE_LOGCAT=FALSE \
    -DSBC_FOR_EMBEDDED_LINUX -DONE_CAM
   # -DSHOW_DEBUG_INFO \

#######################################################################
ifeq ($(MPPCFG_USE_KFC),Y)
    TARGET_STATIC_LIB += lib_hal
endif

ifeq ($(strip $(ENABLE_RTSP)), false)
EXCLUDE_FILES += \
    source/device_model/rtsp.cpp \

TARGET_SRC := $(call filter-out, $(EXCLUDE_FILES), $(TARGET_SRC))
else
# TARGET_STATIC_LIB += libTinyServer
TARGET_SHARED_LIB += libTinyServer
TARGET_CPPFLAGS += -DENABLE_RTSP
endif

ifeq ($(strip $(GUI_SUPPORT)), false)
EXCLUDE_FILES += \
    source/device_model/display.cpp \
    source/device_model/media/video_player.cpp

TARGET_SRC := $(call filter-out, $(EXCLUDE_FILES), $(TARGET_SRC))
TARGET_SRC += \
    source/bll_presenter/sample_ipc.cpp
else
SRC_TAGS := \
    source/minigui-cpp/runtime \
    source/minigui-cpp/data \
    source/minigui-cpp/debug \
    source/minigui-cpp/extra \
    source/minigui-cpp/memory \
    source/minigui-cpp/parser \
    source/minigui-cpp/resource \
    source/minigui-cpp/type \
    source/minigui-cpp/utils \
    source/minigui-cpp/widgets \
    source/minigui-cpp/window \

EXCLUDE_DIRS +=

SRC_TAGS := $(call filter-out, $(EXCLUDE_DIRS), $(SRC_TAGS))

TARGET_SRC += source/minigui-cpp/application.cpp
TARGET_SRC += $(call all-cpp-files-under, $(SRC_TAGS))
TARGET_SRC += $(call all-c-files-under, $(SRC_TAGS))
TARGET_SRC += \
    source/uilayer_view/gui/minigui/window/dialog.cpp \
    source/uilayer_view/gui/minigui/window/preview_window.cpp \
    source/uilayer_view/gui/minigui/window/playlist_window.cpp \
    source/uilayer_view/gui/minigui/window/bind_window.cpp \
    source/uilayer_view/gui/minigui/window/playback_window.cpp \
    source/uilayer_view/gui/minigui/window/status_bar_window.cpp \
    source/uilayer_view/gui/minigui/window/window_manager.cpp \
    source/uilayer_view/gui/minigui/window/newSettingWindow.cpp \
	source/uilayer_view/gui/minigui/window/setting_window_new.cpp \
    source/uilayer_view/gui/minigui/window/promptBox.cpp \
    source/uilayer_view/gui/minigui/window/levelbar.cpp \
    source/uilayer_view/gui/minigui/window/bulletCollection.cpp \
    source/bll_presenter/status_bar.cpp \
    source/bll_presenter/newPreview.cpp \
    source/bll_presenter/event_monitor.cpp \
    source/bll_presenter/binddevice.cpp \
    source/bll_presenter/playback.cpp \
    source/bll_presenter/device_setting.cpp \
    source/bll_presenter/screensaver.cpp \
	source/bll_presenter/statusbarsaver.cpp \
    source/bll_presenter/autoshutdown.cpp \
    source/bll_presenter/status_bar_bottom.cpp \
    source/bll_presenter/setting_handler.cpp \
    source/bll_presenter/closescreen.cpp \
    source/bll_presenter/camRecCtrl.cpp \
    source/bll_presenter/audioCtrl.cpp \
    source/bll_presenter/AdapterLayer.cpp \
    source/bll_presenter/fileLockManager.cpp \
    source/uilayer_view/gui/minigui/window/status_bar_bottom_window.cpp \
    source/uilayer_view/gui/minigui/window/setting_window.cpp \
    source/uilayer_view/gui/minigui/window/usb_mode_window.cpp \
    source/uilayer_view/gui/minigui/window/shutdown_window.cpp \
    source/uilayer_view/gui/minigui/window/time_setting_window.cpp \
	source/uilayer_view/gui/minigui/window/time_setting_window_new.cpp \
    source/uilayer_view/gui/minigui/window/setting_handler_window.cpp \
    source/uilayer_view/gui/minigui/window/prompt.cpp \
	source/uilayer_view/gui/minigui/window/sublist.cpp \
	source/uilayer_view/gui/minigui/window/info_dialog.cpp \
    source/bll_presenter/globalInfo.cpp 
	

ifeq ($(strip $(YI_SMARTDRIVING_ADAS_SUPPORT)), true)
TARGET_SHARED_LIB += \
	libAdas
TARGET_INC += \
	$(TARGET_PATH)/adasApp
TARGET_SRC += \
	adasApp/sample_YiAdas.cpp
endif

TARGET_INC += \
    $(TARGET_PATH)/source/minigui-cpp \

TARGET_SHARED_LIB += \

TARGET_STATIC_LIB += \
    libminigui_ths \
    edog \
    libpng \
    libjpeg \
    libz \
    libqrencode \
    libdd_serv \
    libjsoncpp \
	libpaho-embed-mqtt3cc \
	libpaho-embed-mqtt3c \
	libcurl
TARGET_SHARED_LIB += \
	libts
TARGET_LDFLAGS += \

TARGET_CPPFLAGS += -DGUI_SUPPORT
endif

ifeq ($(strip $(BT_SUPPORT)), false)
EXCLUDE_FILES += \
    source/device_model/system/net/bluetooth_controller.cpp

TARGET_SRC := $(call filter-out, $(EXCLUDE_FILES), $(TARGET_SRC))
else
TARGET_SHARED_LIB += \
    libbtctrl \
    libbt-vendor

ifeq ($(strip $(YI_SMARTDRIVING_WIFI_STA_SUPPORT)), true)
	TARGET_STATIC_LIB += \
		libwifi_sta
	TARGET_INC += \
		$(TARGET_TOP)/system/public/wifi
	endif

TARGET_CPPFLAGS += -DBT_SUPPORT
endif

ifeq ($(strip $(SUB_RECORD_SUPPORT)), true)
TARGET_CPPFLAGS += -DSUB_RECORD_SUPPORT
endif

ifeq ($(strip $(TUTK_SUPPORT)), true)
# TARGET_SHARED_LIB += \
    # libtutk \
    # libAVAPIs \
    # libIOTCAPIs \

TARGET_STATIC_LIB += \
    libtutk \
    libAVAPIs \
    libIOTCAPIs \

TARGET_CPPFLAGS += -DTUTK_SUPPORT
endif

ifeq ($(strip $(ONVIF_SUPPORT)), true)
TARGET_SHARED_LIB += \
   libOnvif

# TARGET_STATIC_LIB += \
    # libOnvif

TARGET_CPPFLAGS += -DONVIF_SUPPORT -DDEBUG
endif

ifeq ($(MPPCFG_VIDEOSTABILIZATION),Y)
TARGET_SHARED_LIB += \
   libIRIDALABS_ViSta
#TARGET_CPPFLAGS += -DENABLE_EIS
endif

ifeq ($(strip $(ENABLE_WATCHDOG)), true)
TARGET_CPPFLAGS += -DENABLE_WATCHDOG
endif

ifeq ($(strip $(BOARD_TYPE)), V7)
TARGET_CPPFLAGS += -DFOX_V7=1
endif

ifeq ($(strip $(BOARD_TYPE)), V8)
TARGET_CPPFLAGS += -DFOX_V8=1
endif

ifeq ($(strip $(BOARD_TYPE)), V9)
TARGET_CPPFLAGS += -DFOX_V9=1
endif

ifeq ($(strip $(BOARD_TYPE)), V10)
TARGET_CPPFLAGS += -DFOX_V10=1
endif

ifeq ($(strip $(BOARD_TYPE)), V11)
TARGET_CPPFLAGS += -DFOX_V11=1
endif

ifeq ($(strip $(BOARD_TYPE)), C26A)
TARGET_CPPFLAGS += -DFOX_C26A=1
endif

ifeq ($(strip $(YI_SMARTDRIVING_SUPPORT)), true)
TARGET_CPPFLAGS += -DYI_SMARTDRIVING_SUPPORT
endif

ifeq ($(strip $(YI_SMARTDRIVING_ADAS_SUPPORT)), true)
TARGET_CPPFLAGS += -DYI_SMARTDRIVING_ADAS_SUPPORT
endif

ifeq ($(strip $(YI_SMARTDRIVING_SUPPORT)), true)
TARGET_CPPFLAGS += -DYI_SMARTDRIVING_IOT_SUPPORT
endif

ifeq ($(strip $(YI_SMARTDRIVING_SUPPORT)), true)
TARGET_CPPFLAGS += -DYI_SMARTDRIVING_IOT_UBER_SUPPORT
endif

ifeq ($(strip $(YI_SMARTDRIVING_WIFI_STA_SUPPORT)), true)
TARGET_CPPFLAGS += -DYI_SMARTDRIVING_WIFI_STA_SUPPORT
endif

TARGET_MODULE := sdvcam
include $(BUILD_BIN)
