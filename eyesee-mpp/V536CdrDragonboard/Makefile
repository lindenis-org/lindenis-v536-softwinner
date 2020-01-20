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
#	make -C apps/cdr/res                           all
#	make -C apps/cdr/config                        all
#	make -C apps/cdr/driver/build.mk               all
#	make -C apps/cdr/patch/build.mk                all
#	make -C apps/cdr/tutk/wrapper/build.mk         all
#	make -C apps/cdr/webserver                     all
#	make -C apps/cdr/webapp                        all
#	make -C apps/cdr -f tina_tools.mk              all
#	make -C apps/cdr/tools/otaUpdate               all
#	make -C apps/cdr/tools/log_guardian            all
#	make -C apps/cdr/tools/gettimems               all
#	make -C apps/cdr/tools/autotest                all
#	make -C apps/cdr/tools/wavplayer/build.mk      all
#	make -C apps/cdr -f tina_sdvcam.mk             all
#	make -C apps/cdr -f tina_sdvcam_preload.mk     all
#	make -C apps/DragonBoard/res                   all
	make -C apps/DragonBoard/testcase/tftester     all
	make -C apps/DragonBoard/testcase/keytester    all
	make -C apps/DragonBoard/testcase/rtctester    all
	make -C apps/DragonBoard/testcase/ddrtester    all
	make -C apps/DragonBoard/testcase/nortester    all
	make -C apps/DragonBoard/testcase/mictester    all
	make -C apps/DragonBoard/testcase/sample_virvi2vo    all
	make -C apps/DragonBoard/testcase/sample_gsensor     all
	make -C apps/DragonBoard/config                all
	make -C apps/DragonBoard/res                   all
	make -C apps/DragonBoard/core -f tina_dragonboard.mk all
	@echo build eyesee-mpp-custom_aw done!

clean:
	@echo ==================================================
	@echo clean eyesee-mpp-custom_aw
	@echo ==================================================
	make -C lib                                       clean
#	make -C lib/minigui-cpp/build.mk                  clean
#	make -C apps/cdr/res                           clean
#	make -C apps/cdr/config                        clean
#	make -C apps/cdr/driver/build.mk               clean
#	make -C apps/cdr/patch/build.mk                clean
#	make -C apps/cdr/tutk/wrapper/build.mk         clean
#   make -C apps/cdr/webserver                     clean
#   make -C apps/cdr/webapp                        clean
#   make -C apps/cdr -f tina_tools.mk              clean
#   make -C apps/cdr/tools/otaUpdate               clean
#   make -C apps/cdr/tools/log_guardian            clean
#   make -C apps/cdr/tools/gettimems               clean
#	make -C apps/cdr/tools/autotest                clean
#	make -C apps/cdr/tools/wavplayer/build.mk      clean
#	make -C apps/cdr -f tina_sdvcam.mk             clean
#	make -C apps/cdr -f tina_sdvcam_preload.mk     clean
	make -C apps/DragonBoard/testcase/tftester     clean
	make -C apps/DragonBoard/testcase/keytester    clean
	make -C apps/DragonBoard/testcase/rtctester    clean
	make -C apps/DragonBoard/testcase/ddrtester    clean
	make -C apps/DragonBoard/testcase/nortester    clean
	make -C apps/DragonBoard/testcase/mictester    clean
	make -C apps/DragonBoard/testcase/sample_virvi2vo    clean
	make -C apps/DragonBoard/testcase/sample_gsensor     clean
	make -C apps/DragonBoard/config                clean
	make -C apps/DragonBoard/res                   clean
	make -C apps/DragonBoard/core -f tina_dragonboard.mk clean
	@echo clean eyesee-mpp-custom_aw done!
