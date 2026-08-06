# Makefile for badge.lgbt-animloader firmware - CLI build using TI ARM CGT +
# XDCtools
#
# Derived from ccs_workspace/badge.lgbt-animloader/.cproject (configuration:
# SingleMode_FlashOnly) and .project (linkedResources / project variables).
# Compiler:  TI ARM Code Generation Tools (armcl) 20.2.5.LTS
# Kernel:    TI-RTOS from simplelink_cc2640r2_sdk_5_10_00_02, configured via
#            XDCtools (xs xdc.tools.configuro) from TOOLS/app_ble.cfg, which
#            just imports the SDK's cc2640_r2_csdk.cfg (BIOS-in-ROM kernel)
# Target:    CC2640R2F (Cortex-M3)
#
# Usage:
#   make                  # configuro + compile + link -> build/badge.lgbt-animloader.out
#   make hex              # also produce Intel-hex image (build/badge.lgbt-animloader.hex)
#   make clean
#   make print-flags      # show full flag set without building
#
# Differences from the main firmware (ccs_workspace/badge.lgbt/Makefile):
#   - --printf_support=minimal is set (the main firmware doesn't set it)
#   - no BOARD_DISPLAY_USE_* defines; FEATURE_SCANNER and FEATURE_OBSERVER are
#     x-prefixed out (broadcast-only uBLE)
#   - no ble/ include dir or ble/ sources; smaller source list (no ir/led/
#     tlc6983 drivers, no adc.c; adds badge_drivers/anims.c)
#   - TOOLS/app_ble.cfg is a one-line import of the SDK's default
#     cc2640_r2_csdk.cfg rather than a project-local kernel config
#
# CCS macro resolutions (from the SDK's .metadata/product.json and .project's
# <variableList>):
#   COM_TI_SIMPLELINK_CC2640R2_SDK_INSTALL_DIR -> $(SDK_ROOT)
#   COM_TI_SIMPLELINK_CC2640R2_SDK_INCLUDE_PATH -> $(SDK_ROOT)/source
#   COM_TI_SIMPLELINK_CC2640R2_SDK_SYMBOLS / _LIBRARIES / _LIBRARY_PATH ->
#       (empty; product.json defines no symbol/library macros)
#   SRC_BLE_DIR      -> $(SDK_ROOT)/source/ti/blestack
#   EXAMPLE_BLE_ROOT -> $(SDK_ROOT)/examples/rtos/CC2640R2_LAUNCHXL/blestack/micro_ble_test

# ---- Toolchain paths -------------------------------------------------------
CGT_ROOT ?= $(HOME)/ti/ti-cgt-arm_20.2.5.LTS
SDK_ROOT ?= $(HOME)/ti/simplelink_cc2640r2_sdk_5_10_00_02
XDC_ROOT ?= $(HOME)/ti/xdctools_3_62_00_08_core

CC  := $(CGT_ROOT)/bin/armcl
HEX := $(CGT_ROOT)/bin/armhex
XS  := $(XDC_ROOT)/xs

BLE_DIR     := $(SDK_ROOT)/source/ti/blestack
EXAMPLE_BLE := $(SDK_ROOT)/examples/rtos/CC2640R2_LAUNCHXL/blestack/micro_ble_test

# ---- Project root (this Makefile lives in ccs_workspace/badge.lgbt-animloader/)
PROJ_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

# ---- Output -----------------------------------------------------------------
BUILD_DIR := $(PROJ_ROOT)build
TARGET    := badge.lgbt-animloader
OUT       := $(BUILD_DIR)/$(TARGET).out
MAP       := $(BUILD_DIR)/$(TARGET).map
XML       := $(BUILD_DIR)/$(TARGET)_linkInfo.xml
HEX_OUT   := $(BUILD_DIR)/$(TARGET).hex

# XDC configuro output package (contains compiler.opt + linker.cmd)
CONFIGPKG := $(BUILD_DIR)/configPkg

# ---- Compiler flags (SingleMode_FlashOnly configuration) --------------------
# CFLAGS_CORE: front-end flags. These are also re-passed on the *link*
# invocation before -z (see the link rule) -- CCS's generated link line does
# the same, and the RTS auto-select mechanism behind the generic `libc.a`
# library name reads target/optimization settings from the flags given to
# that cl invocation, not from the already-compiled objects.
CFLAGS_CORE := \
    -mv7M3 \
    --code_state=16 \
    -me \
    --abi=eabi \
    -O4 \
    --opt_for_speed=0 \
    --c99 \
    --gcc \
    -g \
    --diag_warning=225 \
    --diag_wrap=off \
    --display_error_number \
    --gen_func_subsections=on \
    --printf_support=minimal

# Predefined symbols (.cproject DEFINE list; the leading
# ${COM_TI_SIMPLELINK_CC2640R2_SDK_SYMBOLS} macro expands to nothing).
# The x-prefixed names are CCS-idiom "commented out" defines; they are defined
# verbatim as in the IDE build (harmless -- nothing tests them).
DEFINES := \
    --define=CACHE_AS_RAM \
    --define=Board_EXCLUDE_NVS_INTERNAL_FLASH \
    --define=DeviceFamily_CC26X0R2 \
    --define=CC2640R2_LAUNCHXL \
    --define=CC26XX \
    --define=CC26XX_R2 \
    --define=FEATURE_ADVERTISER \
    --define=FEATURE_BROADCASTER \
    --define=xFEATURE_SCANNER \
    --define=xFEATURE_MONITOR \
    --define=xFEATURE_OBSERVER \
    --define=HEAPMGR_SIZE=0 \
    --define=xICALL_EVENTS \
    --define=POWER_SAVING \
    --define=RF_SINGLEMODE \
    --define=TBM_ACTIVE_ITEMS_ONLY \
    --define=xUSE_ICALL \
    --define=xdc_runtime_Assert_DISABLE_ALL \
    --define=xdc_runtime_Log_DISABLE_ALL

# Include paths (.cproject INCLUDE_PATH list, macros resolved as above)
INCLUDES := \
    --include_path=$(SDK_ROOT)/source \
    --include_path=$(PROJ_ROOT) \
    --include_path=$(PROJ_ROOT)badge_drivers \
    --include_path=$(PROJ_ROOT)Startup \
    --include_path=$(BLE_DIR)/controller/cc26xx_r2/inc \
    --include_path=$(BLE_DIR)/inc \
    --include_path=$(BLE_DIR)/rom \
    --include_path=$(BLE_DIR)/boards \
    --include_path=$(BLE_DIR)/common/cc26xx \
    --include_path=$(EXAMPLE_BLE)/src/app \
    --include_path=$(BLE_DIR)/icall/inc \
    --include_path=$(BLE_DIR)/profiles/roles/cc26xx \
    --include_path=$(BLE_DIR)/profiles/roles \
    --include_path=$(BLE_DIR)/target \
    --include_path=$(BLE_DIR)/hal/src/target/_common \
    --include_path=$(BLE_DIR)/hal/src/target/_common/cc26xx \
    --include_path=$(BLE_DIR)/hal/src/inc \
    --include_path=$(BLE_DIR)/heapmgr \
    --include_path=$(BLE_DIR)/icall/src/inc \
    --include_path=$(BLE_DIR)/osal/src/inc \
    --include_path=$(BLE_DIR)/services/src/saddr \
    --include_path=$(BLE_DIR)/services/src/sdata \
    --include_path=$(SDK_ROOT)/source/ti/devices/cc26x0r2 \
    --include_path=$(CGT_ROOT)/include

# Full compile flag set: application objects additionally pull in the
# XDC-generated include/define set via configPkg/compiler.opt (--cmd_file),
# exactly as CCS does with $(GEN_OPTS__FLAG).
CFLAGS = $(CFLAGS_CORE) $(DEFINES) $(INCLUDES) --cmd_file=$(CONFIGPKG)/compiler.opt

# ---- XDC configuro ----------------------------------------------------------
# Mirrors the .cproject XDCtools tool settings: target ti.targets.arm.elf.M3,
# platform ti.platforms.simplelink:CC2640R2F, release profile, and the
# three-repo --xdcpath. --compileOptions receives the same front-end flags +
# defines the app compiles use (CCS passes ${COMPILER_FLAGS} here).
# NOTE: configuro also (re)generates ROM sysbios build artifacts under the
# project's TOOLS/src/ -- driven by the imported cc2640_r2_csdk.cfg; those
# paths are gitignored.
XDCPATH := $(SDK_ROOT)/source;$(SDK_ROOT)/kernel/tirtos/packages;$(BLE_DIR)
XDC_CFG := $(PROJ_ROOT)TOOLS/app_ble.cfg

# ---- Sources ----------------------------------------------------------------
# All project .c files (.cproject sourceEntries exclude only the NPI/* tree,
# TOOLS/src, and TOOLS/cc26xx_app.cmd). Plus the two linked-resource sources
# from .project:
#   Drivers/RF/RFCC26XX_singleMode.c -> $(SDK)/source/ti/drivers/rf/...
#     (SingleMode config compiles the RF driver from source; note the
#     SingleMode linker library list carries NO rf_*Mode library)
#   HAL/Common/hal_assert.c -> $(BLE_DIR)/hal/src/common/hal_assert.c
PROJ_SRCS := \
    $(PROJ_ROOT)badge.c \
    $(PROJ_ROOT)badge_drivers/anims.c \
    $(PROJ_ROOT)badge_drivers/storage.c \
    $(PROJ_ROOT)Stack/port.c \
    $(PROJ_ROOT)Stack/uble.c \
    $(PROJ_ROOT)Stack/ugap.c \
    $(PROJ_ROOT)Stack/ull.c \
    $(PROJ_ROOT)Stack/urfi.c \
    $(PROJ_ROOT)Startup/board.c \
    $(PROJ_ROOT)Startup/ccfg_app_ble.c \
    $(PROJ_ROOT)Startup/main.c \
    $(PROJ_ROOT)Startup/post.c \
    $(PROJ_ROOT)Startup/urfc.c

SDK_SRCS := \
    $(SDK_ROOT)/source/ti/drivers/rf/RFCC26XX_singleMode.c \
    $(BLE_DIR)/hal/src/common/hal_assert.c

ALL_SRCS := $(PROJ_SRCS) $(SDK_SRCS)

# Flat object directory; all source basenames are unique in this project.
ALL_OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(ALL_SRCS:.c=.obj)))

# ---- Linker flags -----------------------------------------------------------
# IMPORTANT ordering (matches CCS's generated link line):
#   armcl <front-end flags> -z <linker flags> <objects> <libs/cmd files>
# Front-end flags must precede -z; linker-mode flags must follow it.
# Libraries must come AFTER the objects: the TI linker only searches an
# archive for symbols already undefined when it reaches that archive on the
# command line. --reread_libs (a CCS default) lets it iterate for
# cross-archive references.
LFLAGS := \
    -m$(MAP) \
    --heap_size=0 \
    --stack_size=256 \
    -i$(CGT_ROOT)/lib \
    -i$(CGT_ROOT)/include \
    --reread_libs \
    --define=CC26X0ROM=2 \
    --diag_suppress=16002-D \
    --diag_suppress=10247-D \
    --diag_suppress=10325-D \
    --diag_suppress=10229-D \
    --diag_suppress=16032-D \
    --diag_wrap=off \
    --display_error_number \
    --warn_sections \
    --xml_link_info=$(XML) \
    --rom_model \
    -o $(OUT)

# Libraries and linker command files, in .cproject order (the leading
# ${COM_TI_SIMPLELINK_CC2640R2_SDK_LIBRARIES} macro expands to nothing).
# cc26xx_app.cmd supplies memory map + --entry_point ResetISR.
# configPkg/linker.cmd (the sysbios/xdc runtime, including the always-linked
# app_ble_pem3.oem3 object that DEFINES malloc/free/memalign) must precede
# libc.a: if the generic RTS archive is read first it resolves malloc & co.
# from the RTS, and the pem3 object then redefines them -- link error #10056.
LFLAGS_LIBS := \
    -l$(SDK_ROOT)/source/third_party/spiffs/lib/ccs/m3/spiffs_cc26xx.a \
    -l$(SDK_ROOT)/source/ti/devices/cc26x0r2/driverlib/bin/ccs/driverlib.lib \
    -l$(SDK_ROOT)/kernel/tirtos/packages/ti/dpl/lib/dpl_cc26x0r2.aem3 \
    -l$(SDK_ROOT)/source/ti/drivers/lib/drivers_cc26x0r2.aem3 \
    -l$(SDK_ROOT)/source/ti/display/lib/display.aem3 \
    -l$(SDK_ROOT)/source/ti/grlib/lib/ccs/m3/grlib.a \
    -l$(BLE_DIR)/common/cc26xx/ccs/cc26xx_app.cmd \
    -l$(CONFIGPKG)/linker.cmd \
    -llibc.a

# ---- Targets ----------------------------------------------------------------
.PHONY: all clean hex check-toolchain print-flags

all: check-toolchain $(OUT)

hex: check-toolchain $(HEX_OUT)

check-toolchain:
	@if [ ! -x "$(CC)" ]; then \
		echo ""; \
		echo "ERROR: TI ARM CGT compiler not found at: $(CC)"; \
		echo "Install ti-cgt-arm 20.2.5.LTS and set CGT_ROOT, e.g.:"; \
		echo "  make CGT_ROOT=/path/to/ti-cgt-arm_20.2.5.LTS"; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -d "$(SDK_ROOT)/source/ti/blestack" ]; then \
		echo ""; \
		echo "ERROR: SimpleLink CC2640R2 SDK not found at: $(SDK_ROOT)"; \
		echo "Install simplelink_cc2640r2_sdk_5_10_00_02 and set SDK_ROOT."; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -x "$(XS)" ]; then \
		echo ""; \
		echo "ERROR: XDCtools xs not found at: $(XS)"; \
		echo "Install xdctools 3.62 core and set XDC_ROOT."; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -x "$(HEX)" ]; then \
		echo ""; \
		echo "ERROR: TI ARM hex utility not found at: $(HEX)"; \
		echo "'make hex' needs it. A CGT_ROOT that has armcl but not armhex"; \
		echo "is usually a partial install; reinstall ti-cgt-arm 20.2.5.LTS."; \
		echo ""; \
		exit 1; \
	fi

# configuro: regenerated whenever app_ble.cfg changes. compiler.opt is the
# single stamp for the whole generated package (a multi-target rule would run
# the recipe once per target under -j, and two concurrent configuro runs
# corrupt each other's TOOLS/src library build).
$(CONFIGPKG)/linker.cmd: $(CONFIGPKG)/compiler.opt

$(CONFIGPKG)/compiler.opt: $(XDC_CFG) | $(BUILD_DIR)
	cd $(BUILD_DIR) && $(XS) --xdcpath="$(XDCPATH)" xdc.tools.configuro \
		-o configPkg \
		-t ti.targets.arm.elf.M3 \
		-p ti.platforms.simplelink:CC2640R2F \
		-r release \
		-c $(CGT_ROOT) \
		--compileOptions "$(CFLAGS_CORE) $(DEFINES)" \
		$(XDC_CFG)

# ---- Compile rules ----------------------------------------------------------
$(ALL_OBJS): $(CONFIGPKG)/compiler.opt

$(BUILD_DIR)/%.obj: $(PROJ_ROOT)%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(BUILD_DIR)/%.obj: $(PROJ_ROOT)badge_drivers/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(BUILD_DIR)/%.obj: $(PROJ_ROOT)Stack/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(BUILD_DIR)/%.obj: $(PROJ_ROOT)Startup/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(BUILD_DIR)/%.obj: $(SDK_ROOT)/source/ti/drivers/rf/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(BUILD_DIR)/%.obj: $(BLE_DIR)/hal/src/common/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

# ---- Link -------------------------------------------------------------------
$(OUT): $(ALL_OBJS) $(CONFIGPKG)/linker.cmd
	$(CC) $(CFLAGS_CORE) $(DEFINES) -z $(LFLAGS) $(ALL_OBJS) $(LFLAGS_LIBS)

# ---- Hex (postbuild step from .cproject) ------------------------------------
$(HEX_OUT): $(OUT)
	$(HEX) -order MS --memwidth=8 --romwidth=8 --intel -o $(HEX_OUT) $(OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

print-flags:
	@echo "CC:          $(CC)"
	@echo ""
	@echo "CFLAGS:      $(CFLAGS)"
	@echo ""
	@echo "LFLAGS:      $(LFLAGS)"
	@echo ""
	@echo "LFLAGS_LIBS: $(LFLAGS_LIBS)"
	@echo ""
	@echo "ALL_SRCS ($(words $(ALL_SRCS)) files):"
	@for f in $(ALL_SRCS); do echo "  $$f"; done
