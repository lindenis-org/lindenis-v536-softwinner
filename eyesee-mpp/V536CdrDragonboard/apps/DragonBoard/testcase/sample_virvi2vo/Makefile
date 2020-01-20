# Makefile for eyesee-mpp/middleware/sample/sample_vo
CUR_PATH := .
PACKAGE_TOP := ../../../..
EYESEE_MPP_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-mpp
EYESEE_MPP_LIBDIR:=$(STAGING_DIR)/usr/lib/eyesee-mpp
# STAGING_DIR is exported in rules.mk, so it can be used directly here.
# STAGING_DIR:=.../tina-v316/out/v316-perfnor/staging_dir/target

#set source files here.
SRCCS := \
    sample_virvi2vo.c

#include directories
INCLUDE_DIRS := \
   $(PACKAGE_TOP)/include \
   $(EYESEE_MPP_INCLUDE)/system/public/ \
   $(EYESEE_MPP_INCLUDE)/system/public/include \
   $(EYESEE_MPP_INCLUDE)/system/public/liblog \
   $(EYESEE_MPP_INCLUDE)/system/public/include/utils \
   $(EYESEE_MPP_INCLUDE)/middleware/include \
   $(EYESEE_MPP_INCLUDE)/middleware/include/utils \
   $(EYESEE_MPP_INCLUDE)/middleware/include/media \
   $(EYESEE_MPP_INCLUDE)/middleware/media/include \
   $(EYESEE_MPP_INCLUDE)/middleware/media/include/utils \
   $(EYESEE_MPP_INCLUDE)/middleware/media/include/component \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/include \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/include/V4l2Camera \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libisp/isp_tuning \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libcedarx/libcore/common/iniparser \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_FsWriter \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/include_stream \
   $(EYESEE_MPP_INCLUDE)/middleware/media/LIBRARY/libcedarc/include/ \
   $(EYESEE_MPP_INCLUDE)/middleware/sample/configfileparser \

LOCAL_SHARED_LIBS := \
    libsample_confparser \
    librt \
    libpthread \
    liblog \
    libion \
    libhwdisplay \
    libasound \
    libcedarxstream \
    libcutils \
    libcdx_common \
    libcdx_base \
    libcdx_parser \
	
LOCAL_STATIC_LIBS := \
	libaw_mpp \
    libmedia_utils \
    libcedarx_aencoder \
    libaacenc \
    libmp3enc \
    libadecoder \
    libcedarxdemuxer \
	libcedarx_tencoder \
    libvdecoder \
    libvideoengine \
    libawh264 \
    libawh265 \
    libawmjpegplus \
    libvencoder \
    libvenc_codec \
    libvenc_base \
    libMemAdapter \
    libVE \
    libcdc_base \
    libISP \
    libisp_dev \
    libisp_ini \
    libiniparser \
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

#set dst file name: shared library, static library, execute bin.
LOCAL_TARGET_DYNAMIC :=
LOCAL_TARGET_STATIC :=
LOCAL_TARGET_BIN := virvi2vo

#generate include directory flags for gcc.
inc_paths := $(foreach inc,$(filter-out -I%,$(INCLUDE_DIRS)),$(addprefix -I, $(inc))) \
                $(filter -I%, $(INCLUDE_DIRS))
#Extra flags to give to the C compiler
LOCAL_CFLAGS := $(CFLAGS) $(CEDARX_EXT_CFLAGS) $(inc_paths) -fPIC -Wall
#Extra flags to give to the C++ compiler
LOCAL_CXXFLAGS := $(CXXFLAGS) $(CEDARX_EXT_CFLAGS) $(inc_paths) -fPIC -Wall
#Extra flags to give to the C preprocessor and programs that use it (the C and Fortran compilers).
LOCAL_CPPFLAGS := $(CPPFLAGS)
#target device arch: x86, arm
LOCAL_TARGET_ARCH := $(ARCH)
#Extra flags to give to compilers when they are supposed to invoke the linker,‘ld’.
LOCAL_LDFLAGS := $(LDFLAGS)

LIB_SEARCH_PATHS := \
    $(EYESEE_MPP_LIBDIR) \
    $(PACKAGE_TOP)/lib/out

empty:=
space:= $(empty) $(empty)
	
LOCAL_BIN_LDFLAGS := $(LOCAL_LDFLAGS) \
    $(patsubst %,-L%,$(LIB_SEARCH_PATHS)) \
    -Wl,-rpath-link=$(subst $(space),:,$(strip $(LIB_SEARCH_PATHS))) \
    -Wl,-Bstatic \
    -Wl,--start-group $(foreach n, $(LOCAL_STATIC_LIBS), -l$(patsubst lib%,%,$(patsubst %.a,%,$(notdir $(n))))) -Wl,--end-group \
    -Wl,-Bdynamic \
    $(foreach y, $(LOCAL_SHARED_LIBS), -l$(patsubst lib%,%,$(patsubst %.so,%,$(notdir $(y)))))
	
#generate object files
OBJS := $(SRCCS:%=%.o) #OBJS=$(patsubst %,%.o,$(SRCCS))

#add dynamic lib name suffix and static lib name suffix.
target_dynamic := $(if $(LOCAL_TARGET_DYNAMIC),$(addsuffix .so,$(LOCAL_TARGET_DYNAMIC)),)
target_static := $(if $(LOCAL_TARGET_STATIC),$(addsuffix .a,$(LOCAL_TARGET_STATIC)),)

#add *.h prerequisites
-include $(OBJS:%=%.d)

#generate exe file.
.PHONY: all
all: $(LOCAL_TARGET_BIN)
	@echo ===================================
	@echo build eyesee-mpp-custom_aw-apps-ts-sdv-tina_sdvcam done
	@echo ===================================

$(target_dynamic): $(OBJS)
	$(CXX) $+ $(LOCAL_DYNAMIC_LDFLAGS) -o $@
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------

$(target_static): $(OBJS)
	$(AR) -rcs -o $@ $+
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------

$(LOCAL_TARGET_BIN): $(OBJS) $(DEPEND_LIBS)
	$(CXX) $(OBJS) $(LOCAL_BIN_LDFLAGS) -o $@
	@echo ----------------------------
	@echo "finish target: $@"
#	@echo "object files:  $+"
#	@echo "source files:  $(SRCCS)"
	@echo ----------------------------


#patten rules to generate local object files
$(filter %.cpp.o %.cc.o, $(OBJS)): %.o: %
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) -MD -MP -MF $(@:%=%.d) -c -o $@ $<
$(filter %.c.o, $(OBJS)): %.o: %
	$(CC) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -MD -MP -MF $(@:%=%.d) -c -o $@ $<

# clean all
.PHONY: clean
clean:
	-rm -f $(OBJS) $(OBJS:%=%.d) $(target_dynamic) $(target_static) $(LOCAL_TARGET_BIN)

