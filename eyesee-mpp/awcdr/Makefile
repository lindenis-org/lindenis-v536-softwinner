############################################################################################
# 			eyesee-mpp-custom_aw for tina(OpenWrt) Linux
#
#	eyesee-mpp is designed for CDR/SDV product, focus on video/audio capturing and encoding, 
# it also can support video/audio decode.
#   eyesee-mpp-custom is our solutions to some products such as sdv. It mainly uses 
# to implement multimedia function.
#
# Version: v1.0
# Date   : 2019-2-21
# Author : PDC-PD5
############################################################################################
all:
	@echo ==================================================
	@echo build eyesee-mpp-custom_aw
	@echo ==================================================
	make -C lib                                       all
#	make -C lib/minigui-cpp/build.mk                  all
	make -C apps/cdr/res                           all
	make -C apps/cdr/config                        all
#	make -C apps/cdr/driver/build.mk               all
#	make -C apps/cdr/patch/build.mk                all
#	make -C apps/cdr/tutk/wrapper/build.mk         all
	make -C apps/cdr/webserver                     all
	make -C apps/cdr/webapp                        all
	make -C apps/cdr -f tina_tools.mk              all
	make -C apps/cdr/tools/otaUpdate               all
	make -C apps/cdr/tools/log_guardian            all
	make -C apps/cdr/tools/gettimems               all
#	make -C apps/cdr/tools/autotest                all
#	make -C apps/cdr/tools/wavplayer/build.mk      all
	make -C apps/cdr -f tina_sdvcam.mk             all
	make -C apps/cdr -f tina_sdvcam_preload.mk     all
	@echo build eyesee-mpp-custom_aw done!

clean:
	@echo ==================================================
	@echo clean eyesee-mpp-custom_aw
	@echo ==================================================
	make -C lib                                       clean
#	make -C lib/minigui-cpp/build.mk                  clean
	make -C apps/cdr/res                           clean
	make -C apps/cdr/config                        clean
#	make -C apps/cdr/driver/build.mk               clean
#	make -C apps/cdr/patch/build.mk                clean
#	make -C apps/cdr/tutk/wrapper/build.mk         clean
	make -C apps/cdr/webserver                     clean
	make -C apps/cdr/webapp                        clean
	make -C apps/cdr -f tina_tools.mk              clean
	make -C apps/cdr/tools/otaUpdate               clean
	make -C apps/cdr/tools/log_guardian            clean
	make -C apps/cdr/tools/gettimems               clean
#	make -C apps/cdr/tools/autotest                clean
#	make -C apps/cdr/tools/wavplayer/build.mk      clean
	make -C apps/cdr -f tina_sdvcam.mk             clean
	make -C apps/cdr -f tina_sdvcam_preload.mk     clean
	@echo clean eyesee-mpp-custom_aw done!
