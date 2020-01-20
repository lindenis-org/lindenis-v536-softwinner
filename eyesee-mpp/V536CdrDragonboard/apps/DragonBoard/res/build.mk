TARGET_PATH :=$(call my-dir)
include $(ENV_CLEAR)

$(shell cd $(TARGET_PATH)/font;bash sxf_maker.sh 1>script.log 2>&1)

$(info ---------copy resource file---------)
#$(call copy-files-under, \
#	$(TARGET_PATH)/font/*.7z, \
#	$(TARGET_OUT)/target/usr/share/minigui/res/font \
#)

#$(call copy-files-under, \
#	$(TARGET_PATH)/lang/*, \
#	$(TARGET_OUT)/target/usr/share/minigui/res/lang \
#)

#$(call copy-files-under, \
#	$(TARGET_PATH)/images/*, \
#	$(TARGET_OUT)/target/usr/share/minigui/res/images \
#)

$(call copy-files-under, \
	$(TARGET_PATH)/others/*, \
	$(TARGET_OUT)/target/usr/share/minigui/res/others \
)

#$(call copy-files-under, \
#	$(TARGET_PATH)/layout/*, \
#	$(TARGET_OUT)/target/usr/share/minigui/res/layout \
#)

$(call copy-files-under, \
	$(TARGET_PATH)/audio/startup.wav, \
	$(TARGET_OUT)/target/usr/share/minigui/res/audio \
)

#$(call copy-files-under, \
#	$(TARGET_PATH)/ISE/*, \
#	$(TARGET_OUT)/target/usr/share/ISE \
#)

#$(call copy-files-under, \
#	$(TARGET_PATH)/EVE/*, \
#	$(TARGET_OUT)/target/usr/share/EVE \
#)

#$(call copy-files-under, \
#	$(TARGET_PATH)/G2D/*, \
#	$(TARGET_OUT)/target/usr/share/G2D \
#)

$(call copy-files-under, \
	$(TARGET_PATH)/DE/*, \
	$(TARGET_OUT)/target/usr/share/DE \
)
