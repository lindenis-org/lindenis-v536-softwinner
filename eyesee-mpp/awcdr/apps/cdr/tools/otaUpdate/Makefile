# Makefile for eyesee-mpp/custom_aw/apps/ts-sdv/tools/otaUpdate
CUR_PATH := .
PACKAGE_TOP := ../../../..
EYESEE_MPP_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-mpp
EYESEE_MPP_LIBDIR:=$(STAGING_DIR)/usr/lib/eyesee-mpp
EYESEE_MINIGUI_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-minigui
# STAGING_DIR is exported in rules.mk, so it can be used directly here.
# STAGING_DIR:=.../tina-v316/out/v316-perfnor/staging_dir/target

#include $(EYESEE_MPP_INCLUDE)/middleware/config/mpp_config.mk

#set source files here.
SRCCS := \
    utils.cpp \
	xml_parser.cpp \
    OtaUpdate.cpp \
    OtaMain.cpp \
    imgdecode.c \
    sprite_card.c \
    sprite_main.c \
    update4GModule.cpp \
    mtd_opration.c

ifneq (,$(findstring $(strip $(TARGET_PRODUCT)),v316_perfnor v316_sdv))
    PRODUCT_DIR := V5SDVTP
else
    PRODUCT_DIR := V5SDV
endif

#include directories
INCLUDE_DIRS := \
    $(CUR_PATH) \
	$(CUR_PATH)/include \
    $(PACKAGE_TOP)/include \
    $(EYESEE_MINIGUI_INCLUDE) \
    $(EYESEE_MPP_INCLUDE)/system/public/include

LOCAL_SHARED_LIBS := \
    libpthread \
    libdl \
    librt \
	liblog \
    libminigui_ths \
    libpng \
    libjpeg \
    libts \
    libz

LOCAL_STATIC_LIBS :=

#set dst file name: shared library, static library, execute bin.
LOCAL_TARGET_DYNAMIC :=
LOCAL_TARGET_STATIC :=
LOCAL_TARGET_BIN := ota_update

#generate include directory flags for gcc.
inc_paths := $(foreach inc,$(filter-out -I%,$(INCLUDE_DIRS)),$(addprefix -I, $(inc))) \
                $(filter -I%, $(INCLUDE_DIRS))
#Extra flags to give to the C compiler
COMMON_CFLAGS:=
ifeq ($(strip $(BOARD_TYPE)), V7)
    COMMON_CFLAGS += -DVERSION_V7=1
else ifeq ($(strip $(BOARD_TYPE)), V8)
    COMMON_CFLAGS += -DVERSION_V8=1
else ifeq ($(strip $(BOARD_TYPE)), V300)
    COMMON_CFLAGS += -DVERSION_V300=1
endif
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
	@echo build eyesee-mpp-custom_aw-apps-ts-sdv-tools-otaUpdate done
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

