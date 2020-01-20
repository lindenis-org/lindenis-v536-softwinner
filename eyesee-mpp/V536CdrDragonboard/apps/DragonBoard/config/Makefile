# Makefile for eyesee-mpp/custom_aw/apps/ts-sdv/config
CUR_PATH := .
PACKAGE_TOP := ../../..
EYESEE_MPP_INCLUDE:=$(STAGING_DIR)/usr/include/eyesee-mpp
EYESEE_MPP_LIBDIR:=$(STAGING_DIR)/usr/lib/eyesee-mpp
# STAGING_DIR is exported in rules.mk, so it can be used directly here.
# STAGING_DIR:=.../tina-v316/out/v316-perfnor/staging_dir/target

-include $(EYESEE_MPP_INCLUDE)/middleware/config/mpp_config.mk
#set source files here.
SRCCS :=

#include directories
INCLUDE_DIRS :=

LOCAL_SHARED_LIBS :=

LOCAL_STATIC_LIBS :=

#set dst file name: shared library, static library, execute bin.
LOCAL_TARGET_DYNAMIC :=
LOCAL_TARGET_STATIC :=
LOCAL_TARGET_BIN :=

#generate include directory flags for gcc.
inc_paths := $(foreach inc,$(filter-out -I%,$(INCLUDE_DIRS)),$(addprefix -I, $(inc))) \
                $(filter -I%, $(INCLUDE_DIRS))
#Extra flags to give to the C compiler
LOCAL_CFLAGS := $(CFLAGS) $(inc_paths) -fPIC -Wall
#Extra flags to give to the C++ compiler
LOCAL_CXXFLAGS := $(CXXFLAGS) $(inc_paths) -fPIC -Wall
#Extra flags to give to the C preprocessor and programs that use it (the C and Fortran compilers).
LOCAL_CPPFLAGS := $(CPPFLAGS)
#target device arch: x86, arm
LOCAL_TARGET_ARCH := $(ARCH)
#Extra flags to give to compilers when they are supposed to invoke the linker,‘ld’.
LOCAL_LDFLAGS := $(LDFLAGS)

LOCAL_DYNAMIC_LDFLAGS := $(LOCAL_LDFLAGS) -shared \
    -L $(EYESEE_MPP_LIBDIR) \
    -Wl,-Bstatic \
    -Wl,--start-group $(foreach n, $(LOCAL_STATIC_LIBS), -l$(patsubst lib%,%,$(patsubst %.a,%,$(notdir $(n))))) -Wl,--end-group \
    -Wl,-Bdynamic \
    $(foreach y, $(LOCAL_SHARED_LIBS), -l$(patsubst lib%,%,$(patsubst %.so,%,$(notdir $(y)))))

#generate object files
OBJS := $(SRCCS:%=%.o) #OBJS=$(patsubst %,%.o,$(SRCCS))

#add dynamic lib name suffix and static lib name suffix.
target_dynamic := $(if $(LOCAL_TARGET_DYNAMIC),$(LOCAL_TARGET_DYNAMIC).so,)
target_static := $(if $(LOCAL_TARGET_STATIC),$(LOCAL_TARGET_STATIC).a,)

#generate exe file.
.PHONY: all
all:
	mkdir -p $(CUR_PATH)/out/etc
	-cp -fpR $(CUR_PATH)/MiniGUI.cfg $(CUR_PATH)/out/etc/MiniGUI.cfg
	-cp -fpR $(CUR_PATH)/sunxi-keyboard.kl $(CUR_PATH)/out/etc/sunxi-keyboard.kl
	-cp -fpR $(CUR_PATH)/ts.conf $(CUR_PATH)/out/etc/ts.conf
	#mkdir -p $(CUR_PATH)/out/etc/udev/rules.d
	#-cp -fpR $(CUR_PATH)/10-local.rules $(CUR_PATH)/out/etc/udev/rules.d
	#-cp -fpR $(CUR_PATH)/udhcpd.conf $(CUR_PATH)/out/etc/udhcpd.conf
	#mkdir -p $(CUR_PATH)/out/usr/share/app/sdv
	#-cp -fpR $(CUR_PATH)/$(BOARD_TYPE)/data/*.lua $(CUR_PATH)/out/usr/share/app/sdv
	#mkdir -p $(CUR_PATH)/out/data
	#-cp -fpR $(CUR_PATH)/$(BOARD_TYPE)/data/* $(CUR_PATH)/out/data
	#$(CUR_PATH)/update_build_info.sh $(CUR_PATH)
	#-mv -f $(CUR_PATH)/build_info.lua $(CUR_PATH)/out/usr/share/app/sdv/build_info.lua
	mkdir -p $(CUR_PATH)/out/etc/init.d
	$(CUR_PATH)/user_config.sh $(CUR_PATH)
	-mv -f $(CUR_PATH)/S01app ${CUR_PATH}/out/etc/init.d/S01app
	@echo ===================================
	@echo build eyesee-mpp-custom_aw-apps-ts-sdv-config done
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

#patten rules to generate local object files
%.cpp.o: %.cpp
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) -c -o $@ $<
%.cc.o: %.cc
	$(CXX) $(LOCAL_CXXFLAGS) $(LOCAL_CPPFLAGS) -c -o $@ $<

%.c.o: %.c
	$(CC) $(LOCAL_CFLAGS) $(LOCAL_CPPFLAGS) -c -o $@ $<

# clean all
.PHONY: clean
clean:
	-rm -f $(OBJS) $(target_dynamic) $(target_static)
	-rm -rf $(CUR_PATH)/out
