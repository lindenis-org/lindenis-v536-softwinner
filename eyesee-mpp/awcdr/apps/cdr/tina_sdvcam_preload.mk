#
# This file is adapt for fast-boot mode.
# It will link the same libraries as sdvcam to accelerate boot time,
#so you should keep them have the same libs when compile.
#
# Makefile for eyesee-mpp/custom_aw/apps/ts-sdv/tina_sdvcam_preload
CUR_PATH := .
PACKAGE_TOP := ../..
EYESEE_MPP_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-mpp
EYESEE_MPP_LIBDIR:=$(STAGING_DIR)/usr/lib/eyesee-mpp
# STAGING_DIR is exported in rules.mk, so it can be used directly here.
# STAGING_DIR:=.../tina-v316/out/v316-perfnor/staging_dir/target

include $(CUR_PATH)/app_config.mk
-include $(EYESEE_MPP_INCLUDE)/middleware/config/mpp_config.mk

#set source files here.
SRCCS := \
    sdvcam_preload.c

#include directories
INCLUDE_DIRS := \
    $(CUR_PATH)

LOCAL_SHARED_LIBS := \
    libpthread \
    libdl \
    librt \
    libcdx_base \
    libcdx_parser \
    libcdx_stream \
    libcdx_common \
    libminigui_ths \
    libasound \
    liblog \
    libpng \
    libjpeg \
    libts \
    libz \
    libuvoice_wakeup \
    libremove_click

ifeq ($(MPPCFG_MOD),Y)
LOCAL_SHARED_LIBS += \
    libai_MOD \
    libCVEMiddleInterface
endif
ifeq ($(MPPCFG_EVEFACE),Y)
LOCAL_SHARED_LIBS += libeve_event
endif
ifeq ($(MPPCFG_VLPR),Y)
LOCAL_SHARED_LIBS += \
    libai_VLPR \
    libCVEMiddleInterface
endif

LOCAL_STATIC_LIBS :=

COMMON_CFLAGS := \
    $(CEDARX_EXT_CFLAGS) \
    -Wno-write-strings \
    -Wno-unused-local-typedefs \
    -Wno-sign-compare \
    -Wno-pointer-arith \
    -fexceptions \
    -DHAS_BDROID_BUILDCFG -DLINUX_NATIVE -DANDROID_USE_LOGCAT=FALSE \
    -DDATABASE_IN_HOTPLUG_STORAGE -DSBC_FOR_EMBEDDED_LINUX -DONE_CAM \
    -DDEBUG_NOTIFY -DDEBUG_FIFO -DAUTO_TEST\
    -DCUT_HDMI_DISPLAY=2 \
    # -DSHOW_DEBUG_INFO

ifeq ($(MPPCFG_USE_KFC),Y)
LOCAL_STATIC_LIBS += lib_hal
endif

ifeq ($(MPPCFG_VIDEOSTABILIZATION),Y)
LOCAL_SHARED_LIBS += \
    libIRIDALABS_ViSta
COMMON_CFLAGS += -DENABLE_EIS
endif

#set dst file name: shared library, static library, execute bin.
LOCAL_TARGET_DYNAMIC :=
LOCAL_TARGET_STATIC :=
ifeq ($(BOARD_BOOT_TYPE),fast)
LOCAL_TARGET_BIN := sdvcam_preload
endif

#generate include directory flags for gcc.
inc_paths := $(foreach inc,$(filter-out -I%,$(INCLUDE_DIRS)),$(addprefix -I, $(inc))) \
                $(filter -I%, $(INCLUDE_DIRS))
#Extra flags to give to the C compiler
LOCAL_CFLAGS := $(CFLAGS) $(inc_paths) -fPIC -Wall $(COMMON_CFLAGS)
#Extra flags to give to the C++ compiler
LOCAL_CXXFLAGS := $(CXXFLAGS) $(inc_paths) -fPIC -Wall $(COMMON_CFLAGS)
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

#generate exe file.
.PHONY: all
all: $(LOCAL_TARGET_BIN)
	@echo ===================================
	@echo build eyesee-mpp-custom_aw-apps-ts-sdv-tina_sdvcam_preload done
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

$(LOCAL_TARGET_BIN): $(OBJS)
	$(CXX) $+ $(LOCAL_BIN_LDFLAGS) -o $@
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

#add *.h prerequisites
-include $(OBJS:%=%.d)

