TARGET_PATH :=$(call my-dir)
include $(ENV_CLEAR)

# $(shell $(TARGET_PATH)/prelink.sh $(TARGET_PATH))
$(shell $(TARGET_PATH)/upx $(TARGET_OUT)/target/usr/bin/sdvcam 1>&2 )
