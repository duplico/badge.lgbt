# Makefile for the badge.lgbt controller dongle firmware - CLI build
#
# Target: MSP430FR2433 (bare-metal, single translation unit: main.c)
# Compiler: TI MSP430 CGT (cl430). The .ccsproject pins 20.2.5.LTS; this
# build uses 21.6.2.LTS (the publicly downloadable LTS installer, same
# precedent as qc2024's CLI build vs its 21.6.1.LTS pin). Flags are derived
# from .cproject (Debug and Release configurations differ only in name; no
# OPT_LEVEL is set in either, so no --opt_level flag is passed).
#
# Device-support files (msp430fr2433.h, in430.h, msp430fr2433.cmd, libmath.a)
# are committed in ccs_base_files/ — the minimal closure from a CCS install's
# ccs_base/msp430/, checksummed in tools/toolchain.lock. No CCS install is
# needed to build.
#
# Usage:
#   make CGT_ROOT=$$HOME/ti/ti-cgt-msp430_21.6.2.LTS
#   make hex          # also produce the TI-TXT image MSP430Flasher wants
#   make print-flags
#   make clean

# ---- Toolchain -------------------------------------------------------------
# CGT_ROOT: root of the standalone TI MSP430 CGT installation. The installer
# creates one versioned subdirectory under --prefix, so with
# `--prefix $HOME/ti` the root is $HOME/ti/ti-cgt-msp430_21.6.2.LTS.
CGT_ROOT ?= $(HOME)/ti/ti-cgt-msp430_21.6.2.LTS

CC  := $(CGT_ROOT)/bin/cl430
HEX := $(CGT_ROOT)/bin/hex430

# ---- Layout ----------------------------------------------------------------
PROJ_ROOT := $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
DEV_DIR   := $(PROJ_ROOT)ccs_base_files
LOCK      := $(PROJ_ROOT)tools/toolchain.lock

BUILD_DIR := $(PROJ_ROOT)build
TARGET    := badge.lgbt-dongle
OUT       := $(BUILD_DIR)/$(TARGET).out
MAP       := $(BUILD_DIR)/$(TARGET).map
XML       := $(BUILD_DIR)/$(TARGET)_linkInfo.xml
# MSP430Flasher picks its parser from the extension: .txt is TI-TXT,
#  .a43/.hex is Intel-Hex. The image below is TI-TXT, so it must be .txt.
HEX_OUT   := $(BUILD_DIR)/$(TARGET).txt

# ---- Compiler flags (derived from .cproject) --------------------------------
CFLAGS := \
    --silicon_version=mspx \
    --use_hw_mpy=F5 \
    --define=__MSP430FR2433__ \
    --include_path=$(DEV_DIR)/include \
    --include_path=$(CGT_ROOT)/include \
    --include_path=$(PROJ_ROOT) \
    --symdebug:dwarf \
    --printf_support=minimal \
    --diag_warning=225 \
    --diag_wrap=off \
    --display_error_number \
    --silicon_errata=CPU21 \
    --silicon_errata=CPU22 \
    --silicon_errata=CPU40

# ---- Link step -------------------------------------------------------------
# LFLAGS_FRONTEND: compiler-mode flags re-passed to the *link* invocation,
# before -z, mirroring CCS's generated link line. The RTS auto-select
# mechanism (triggered by the generic libc.a library name) reads these from
# this cl430 invocation, not from the .obj files — keep this a mirror of
# CFLAGS (minus --include_path) whenever CFLAGS changes.
LFLAGS_FRONTEND := \
    --silicon_version=mspx \
    --use_hw_mpy=F5 \
    --define=__MSP430FR2433__ \
    --symdebug:dwarf \
    --printf_support=minimal \
    --diag_warning=225 \
    --diag_wrap=off \
    --display_error_number \
    --silicon_errata=CPU21 \
    --silicon_errata=CPU22 \
    --silicon_errata=CPU40

# --rom_model: required for the RTS _c_int00 auto-init entry point; without
# it the link "succeeds" with the RESET vector unfilled ("no suitable
# entry-point found; setting to 0") and main() dropped.
LFLAGS := \
    --rom_model \
    --search_path=$(DEV_DIR)/include \
    --search_path=$(DEV_DIR)/lib/5xx_6xx_FRxx \
    --search_path=$(CGT_ROOT)/lib \
    --search_path=$(CGT_ROOT)/include \
    --use_hw_mpy=F5 \
    --cinit_hold_wdt=on \
    --heap_size=160 \
    --stack_size=160 \
    --map_file=$(MAP) \
    --output_file=$(OUT) \
    --xml_link_info=$(XML) \
    --diag_wrap=off \
    --display_error_number \
    --warn_sections

# Libraries must come after the objects and linker command file on the cl430
# command line: the TI linker only searches an archive for symbols already
# undefined when it reaches that archive.
# Order matches .cproject: an archive only satisfies symbols still undefined
# when the linker reaches it, so the sequence is part of the configuration.
LFLAGS_LIBS := \
    --library=libmath.a \
    --library=libc.a

# ---- Sources ---------------------------------------------------------------
SRCS    := $(PROJ_ROOT)main.c
OBJS    := $(BUILD_DIR)/main.obj
LNK_CMD := $(PROJ_ROOT)lnk_msp430fr2433.cmd

# ---- Rules -----------------------------------------------------------------
.PHONY: all hex clean check-toolchain print-flags

all: check-toolchain $(OUT)

hex: check-toolchain $(HEX_OUT)

check-toolchain:
	@if [ ! -x "$(CC)" ]; then \
		echo ""; \
		echo "ERROR: TI MSP430 CGT compiler not found at: $(CC)"; \
		echo ""; \
		echo "Install ti-cgt-msp430 21.6.2.LTS (installer URL and checksum in"; \
		echo "tools/toolchain.lock) and set CGT_ROOT, e.g.:"; \
		echo "  make CGT_ROOT=\$$HOME/ti/ti-cgt-msp430_21.6.2.LTS"; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -x "$(HEX)" ]; then \
		echo ""; \
		echo "ERROR: TI MSP430 hex utility not found at: $(HEX)"; \
		echo "'make hex' needs it to emit the TI-TXT image. A CGT_ROOT with"; \
		echo "cl430 but no hex430 is usually a partial install."; \
		echo ""; \
		exit 1; \
	fi
	@if [ ! -f "$(LOCK)" ]; then \
		echo ""; \
		echo "ERROR: $(LOCK) is missing. It records the SHA-256 checksums the"; \
		echo "committed ccs_base_files/ are verified against; without it the build"; \
		echo "cannot confirm those files are what they claim to be."; \
		echo ""; \
		exit 1; \
	fi
	@if ! (cd $(PROJ_ROOT) && grep -E '^[0-9a-f]{64}  ccs_base_files/' tools/toolchain.lock | sha256sum --check --quiet -); then \
		echo ""; \
		echo "ERROR: one or more committed files under ccs_base_files/ do not match"; \
		echo "the SHA-256 checksums recorded in tools/toolchain.lock (see the"; \
		echo "FAILED/mismatch lines above for which). A partial checkout, a moved"; \
		echo "PROJ_ROOT, or local edits to the vendored device-support files all"; \
		echo "look like this. Restore them with 'git checkout -- ccs_base_files/'"; \
		echo "or re-copy them from a CCS install per tools/toolchain.lock's header."; \
		echo ""; \
		exit 1; \
	fi

$(BUILD_DIR)/main.obj: $(PROJ_ROOT)main.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) --output_file=$@ $<

$(OUT): $(OBJS) $(LNK_CMD)
	$(CC) $(LFLAGS_FRONTEND) -z $(LFLAGS) $(OBJS) $(LNK_CMD) $(LFLAGS_LIBS)

$(HEX_OUT): $(OUT)
	$(HEX) --romwidth=8 --memwidth=8 --ti_txt --outfile=$(HEX_OUT) $(OUT)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

print-flags:
	@echo "CC:              $(CC)"
	@echo "CFLAGS:          $(CFLAGS)"
	@echo ""
	@echo "LFLAGS_FRONTEND: $(LFLAGS_FRONTEND)"
	@echo ""
	@echo "LFLAGS:          $(LFLAGS)"
	@echo ""
	@echo "LFLAGS_LIBS:     $(LFLAGS_LIBS)"
