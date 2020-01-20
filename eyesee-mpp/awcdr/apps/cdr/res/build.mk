TARGET_PATH :=$(call my-dir)
include $(ENV_CLEAR)

$(shell cd $(TARGET_PATH)/$(BOARD_TYPE)/font;bash sxf_maker.sh 1>script.log 2>&1)

$(info ---------copy resource file---------)
$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/font/*.7z, \
	$(TARGET_OUT)/target/usr/share/minigui/res/font \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/lang/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/lang \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/statusbar_bottom/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/statusbar_top/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/preivew/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/playback/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/setting/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/others/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/images/unkown/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/images \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/others/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/others \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/layout/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/layout \
)

$(call copy-files-under, \
	$(TARGET_PATH)/$(BOARD_TYPE)/audio/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/audio \
)

