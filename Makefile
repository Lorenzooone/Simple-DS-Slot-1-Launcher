# SPDX-License-Identifier: CC0-1.0
#
# SPDX-FileContributor: Antonio Niño Díaz, 2023

BLOCKSDS	?= /opt/blocksds/core
BLOCKSDSEXT	?= /opt/blocksds/external

# User config
# ===========

NAME		:= slot1launch

GAME_TITLE	:= Slot-1 Launcher
GAME_SUBTITLE	:= Launch the Cartridge in Slot 1
GAME_AUTHOR	:= Lorenzooone
GAME_ICON	:= icon.bmp

# DLDI and internal SD slot of DSi
# --------------------------------

# Root folder of the SD image
SDROOT		:= sdroot
# Name of the generated image it "DSi-1.sd" for no$gba in DSi mode
SDIMAGE		:= image.bin

# Source code paths
# -----------------

# A single directory that is the root of NitroFS:
NITROFSDIR	:=

# Tools
# -----

MAKE		:= make
RM		:= rm -rf

# Verbose flag
# ------------

ifeq ($(VERBOSE),1)
V		:=
else
V		:= @
endif

# Directories
# -----------

ARM9DIR		:= arm9
ARM7DIR		:= arm7

# Build artfacts
# --------------

ROM		:= $(NAME).nds
ROM_dsi		:= $(NAME).dsi

# Targets
# -------

.PHONY: all clean arm9 arm7 cardengine_arm7 bootloader bootloaderAlt dldipatch sdimage

all: $(ROM) $(ROM_dsi)

clean:
	@echo "  CLEAN"
	$(V)$(MAKE) -C arm9 clean --no-print-directory
	$(V)$(MAKE) -C arm7 clean --no-print-directory
	$(V)$(MAKE) -f Makefile.arm7 -C bootloader clean --no-print-directory
	$(V)$(MAKE) -C bootloaderAlt clean --no-print-directory
	$(V)$(MAKE) -C cardengine_arm7 clean --no-print-directory
	$(V)$(RM) $(ROM) $(ROM_dsi) build $(SDIMAGE)

arm9: cardengine_arm7 bootloader bootloaderAlt
	$(V)+$(MAKE) -C arm9 --no-print-directory

arm7: cardengine_arm7 bootloader bootloaderAlt
	$(V)+$(MAKE) -C arm7 --no-print-directory

bootloader: cardengine_arm7
	$(V)+$(MAKE) -f Makefile.arm7 -C bootloader --no-print-directory

bootloaderAlt: cardengine_arm7
	$(V)+$(MAKE) -C bootloaderAlt --no-print-directory

cardengine_arm7:
	$(V)+$(MAKE) -C cardengine_arm7 --no-print-directory

ifneq ($(strip $(NITROFSDIR)),)
# Additional arguments for ndstool
NDSTOOL_ARGS	:= -d $(NITROFSDIR)

# Make the NDS ROM depend on the filesystem only if it is needed
$(ROM): $(NITROFSDIR)
# Make the DSi ROM depend on the filesystem only if it is needed
$(ROM_dsi): $(NITROFSDIR)
endif

# Combine the title strings
ifeq ($(strip $(GAME_SUBTITLE)),)
    GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_AUTHOR)
else
    GAME_FULL_TITLE := $(GAME_TITLE);$(GAME_SUBTITLE);$(GAME_AUTHOR)
endif

$(ROM): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 arm7/build/arm7.elf -9 arm9/build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)" \
		$(NDSTOOL_FAT)

$(ROM_dsi): arm9 arm7
	@echo "  NDSTOOL $@"
	$(V)$(BLOCKSDS)/tools/ndstool/ndstool -c $@ \
		-7 arm7/build/arm7.elf -9 arm9/build/arm9.elf \
		-b $(GAME_ICON) "$(GAME_FULL_TITLE)" \
		-g SL1L 00 "SLOT1LAUNCH" -z 93FFFB06h -u 00030004 \
		$(NDSTOOL_FAT)

sdimage:
	@echo "  MKFATIMG $(SDIMAGE) $(SDROOT)"
	$(V)$(BLOCKSDS)/tools/mkfatimg/mkfatimg -t $(SDROOT) $(SDIMAGE)

dldipatch: $(ROM)
	@echo "  DLDIPATCH $(ROM)"
	$(V)$(BLOCKSDS)/tools/dldipatch/dldipatch patch \
		$(BLOCKSDS)/sys/dldi_r4/r4tf.dldi $(ROM)
