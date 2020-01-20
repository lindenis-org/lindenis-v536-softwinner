target_module:= $(strip $(TARGET_MODULE))
echo "nihao"
NAME:cdr
# early depend, so build first
MODULE:system/public/liblog/build.mk
MODULE:system/public/logcat/build.mk
MODULE:system/public/display/build.mk
#MODULE:system/public/database/build.mk
MODULE:system/public/newfs_msdos/build.mk
MODULE:system/public/reboot_efex/build.mk
MODULE:external/lz4-1.7.5/build.mk
MODULE:external/SQLiteCpp/build.mk
MODULE:external/fsck_msdos/build.mk
MODULE:external/civetweb/build.mk
#MODULE:system/private/onvif/build.mk
# MODULE:system/public/sigsegv/build.mk
MODULE:system/public/luaconfig/build.mk

MODULE:system/public/tinyalsa/build.mk
MODULE:system/public/tinyalsa/utils/build.mk

#bluetooth
# MODULE:system/public/bt/fw/build.mk
# MODULE:system/public/bt/lib/build.mk
# MODULE:system/public/bt/demo/socket_test/build.mk

#wifi
MODULE:system/public/wifi/wpa_supplicant/build.mk
MODULE:system/public/wifi/build.mk

#4g
#MODULE:system/public/usb4g/build.mk

#rgb
MODULE:system/public/rgb_ctrl/build.mk

#smartlink
#MODULE:system/public/smartlink/build.mk

#MODULE:system/public/ntpclient/build.mk

# prebuild lib
MODULE:custom_aw/lib/build.mk
# MODULE:custom_aw/lib/minigui-cpp/build.mk
MODULE:framework/utils/build.mk
MODULE:framework/media/camera/build.mk
# MODULE:framework/media/ise/build.mk
MODULE:framework/media/recorder/build.mk
MODULE:framework/media/player/build.mk
MODULE:framework/media/thumbretriever/build.mk
MODULE:framework/media/videoresizer/build.mk
MODULE:custom_aw/apps/cdr/tools/otaUpdate/build.mk
MODULE:custom_aw/lib/build.mk
MODULE:custom_aw/apps/cdr/res/build.mk
MODULE:custom_aw/apps/cdr/config/build.mk
MODULE:custom_aw/apps/cdr/tutk/wrapper/build.mk
MODULE:custom_aw/apps/cdr/tools.mk
MODULE:custom_aw/apps/cdr/sdvcam.mk
MODULE:custom_aw/apps/cdr/webserver/build.mk
MODULE:custom_aw/apps/cdr/webapp/build.mk
MODULE:custom_aw/apps/cdr/tools/wavplayer/build.mk
MODULE:custom_aw/apps/cdr/tools/host/build.mk
MODULE:custom_aw/apps/cdr/tools/gettimems/build.mk
MODULE:custom_aw/apps/cdr/tools/log_guardian/build.mk
#MODULE:custom_aw/demo/demo_camera/build.mk
#MODULE:custom_aw/demo/RecordDemo/build.mk
#MODULE:custom_aw/demo/cdrPlayerDemo/build.mk
#MODULE:custom_aw/demo/sample_Camera/build.mk
#MODULE:custom_aw/demo/sample_EncodeResolutionChange/build.mk
#MODULE:custom_aw/demo/sample_RecorderCallbackOut/build.mk
#MODULE:custom_aw/demo/sample_RecorderSegment/build.mk
#MODULE:custom_aw/demo/sample_Player/build.mk
#MODULE:framework/demo/sample_YiAdas/build.mk
include $(COMMON_MAKEFILE)

