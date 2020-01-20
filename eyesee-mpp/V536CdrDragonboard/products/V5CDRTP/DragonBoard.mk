NAME:DragonBoard
# early depend, so build first
MODULE:system/public/liblog/build.mk
#MODULE:system/public/logcat/build.mk
MODULE:system/public/display/build.mk
#MODULE:system/public/database/build.mk
MODULE:system/public/newfs_msdos/build.mk
MODULE:system/public/reboot_efex/build.mk
MODULE:external/lz4-1.7.5/build.mk
MODULE:external/SQLiteCpp/build.mk
#MODULE:system/private/rtsp/build.mk
#MODULE:system/private/onvif/build.mk
# MODULE:system/public/sigsegv/build.mk
MODULE:system/public/luaconfig/build.mk

MODULE:system/public/tinyalsa/build.mk
#MODULE:system/public/tinyalsa/utils/build.mk


#wifi
MODULE:system/public/wifi/wpa_supplicant/build.mk
MODULE:system/public/wifi/build.mk

#4g
MODULE:system/public/usb4g/build.mk

#rgb
MODULE:system/public/rgb_ctrl/build.mk

#smartlink
#MODULE:system/public/smartlink/build.mk

#MODULE:system/public/ntpclient/build.mk

# prebuild lib
MODULE:custom_aw/lib/build.mk
#MODULE:custom_aw/lib/minigui-cpp/build.mk
MODULE:framework/utils/build.mk
MODULE:framework/media/camera/build.mk
#MODULE:framework/media/ise/build.mk
MODULE:framework/media/recorder/build.mk
MODULE:framework/media/player/build.mk
MODULE:framework/media/thumbretriever/build.mk
MODULE:framework/media/videoresizer/build.mk

MODULE:custom_aw/apps/DragonBoard/mpp_comm/build.mk
MODULE:custom_aw/apps/DragonBoard/build.mk
MODULE:custom_aw/apps/DragonBoard/core/build.mk


MODULE:custom_aw/apps/DragonBoard/res/build.mk
MODULE:custom_aw/apps/DragonBoard/config/build.mk
MODULE:custom_aw/apps/DragonBoard/lib/build.mk
MODULE:custom_aw/apps/DragonBoard/testcase/build.mk

