/*
    NitroHax -- Cheat tool for the Nintendo DS
    Copyright (C) 2008  Michael "Chishm" Chisholm

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <nds.h>

#include "load_bin.h"
#include "loadAlt_bin.h"
#include "cardengine_arm7_bin.h"
#include "cardengine_arm7_isne_bin.h"
#include "cardengine_arm7_dsi_exp_ram_bin.h"
#include "launch_engine.h"
#include "tonccpy.h"
#include "min_font_bin.h"

#include "cardengine_defs.h"
#include "bootloader_defs.h"
#include "utils.h"

#define LCDC_BANK_D ((u16*)0x06860000)

__attribute__((noreturn)) void runLaunchEngine(struct launch_engine_data_t* launch_engine_data, bool altBootloader, uint32_t boot_type, char* boot_path, bool is_dsi_cart)
{
	bool pass_min_font = true;
	#ifndef DO_BOOTLOADER_DEBUG_PRINTS
	pass_min_font = false;
	#endif
	int dsi_mode_enabled = 1;
	if(!isDSiMode())
		dsi_mode_enabled = 0;

	const uint8_t* chosen_cardengine = dsi_mode_enabled ? cardengine_arm7_bin : NULL;
	size_t chosen_cardengine_size = dsi_mode_enabled ? cardengine_arm7_bin_size : 0;
	if(isRAMDoubled()) {
		if(!dsi_mode_enabled) {
			chosen_cardengine = cardengine_arm7_isne_bin;
			chosen_cardengine_size = cardengine_arm7_isne_bin_size;
		}
		if(dsi_mode_enabled && is_dsi_cart && (launch_engine_data->twlmode != 0) && (!altBootloader)) {
			chosen_cardengine = cardengine_arm7_dsi_exp_ram_bin;
			chosen_cardengine_size = cardengine_arm7_dsi_exp_ram_bin_size;
		}
	}

	const struct cardengine_main_data_t base_empty_cardengine_data = {0};
	struct cardengine_main_data_t cardengine_data = (chosen_cardengine == NULL) ? base_empty_cardengine_data : *((struct cardengine_main_data_t*)chosen_cardengine);
	cardengine_data.boot_type = boot_type;

	const uint8_t* chosen_bootloader = altBootloader ? loadAlt_bin : load_bin;
	size_t chosen_bootloader_size = altBootloader ? loadAlt_bin_size : load_bin_size;
	struct bootloader_main_data_t load_data = *((struct bootloader_main_data_t*)chosen_bootloader);

	// Set the parameters for the loader
	load_data.dsiMode = dsi_mode_enabled;
	load_data.language = launch_engine_data->language;
	load_data.sdAccess = launch_engine_data->sdaccess;
	load_data.scfgUnlock = launch_engine_data->scfgUnlock;
	load_data.twlMode = launch_engine_data->twlmode;
	load_data.twlClock = launch_engine_data->twlclk;
	load_data.boostVram = launch_engine_data->twlvram;
	load_data.twlTouch = launch_engine_data->twltouch;
	load_data.soundFreq = launch_engine_data->soundFreq;
	load_data.runCardEngine = launch_engine_data->runCardEngine;
	load_data.sleepMode = launch_engine_data->sleepMode;
	load_data.redirectPowerButton = launch_engine_data->redirectPowerButton;
	load_data.hasDoubleRAM = isRAMDoubled();
	load_data.cardEngineLocation = ((load_data.copy_end + 3) >> 2) << 2;
	load_data.cardEngineSize = chosen_cardengine_size;
	for(size_t i = 0; i < 8; i++)
		load_data.selfTitleId[i] = ((uint8_t*)&__DSiHeader->tid_low)[i];

	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM bank D
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;

	// Clear VRAM
	toncset (LCDC_BANK_D, 0, 128 * 1024);

	// Load the loader/patcher into the correct address
	tonccpy (LCDC_BANK_D, chosen_bootloader, chosen_bootloader_size);

	tonccpy (LCDC_BANK_D, &load_data, sizeof(load_data));

	u8* cardengine_target_ptr = (u8*)(LCDC_BANK_D + ((load_data.cardEngineLocation - load_data.copy_location) / sizeof(LCDC_BANK_D[0])));
	if(chosen_cardengine != NULL)
		tonccpy(cardengine_target_ptr, chosen_cardengine, chosen_cardengine_size);

	// This has important information, copy it regardless
	tonccpy (cardengine_target_ptr, &cardengine_data, sizeof(cardengine_data));
	if(boot_path != NULL)
		tonccpy(cardengine_target_ptr + cardengine_data.boot_path_offset, boot_path, cardengine_data.boot_path_max_len);
	else
		toncset(cardengine_target_ptr + cardengine_data.boot_path_offset, 0, cardengine_data.boot_path_max_len);

	irqDisable(IRQ_ALL);

	if ((isDSiMode()) && altBootloader) {
		if (launch_engine_data->scfgUnlock) {
			REG_SCFG_EXT = launch_engine_data->twlvram ? 0x8300E000 : 0x8300C000;
		} else {
			REG_SCFG_EXT = launch_engine_data->twlvram ? 0x83002000 : 0x83000000;
			REG_SCFG_EXT &= ~(1UL << 31);
		}
	}

	// Give the VRAM to the ARM7
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;

	if(pass_min_font) {
		VRAM_A_CR = VRAM_ENABLE;
		vramSetBankA(VRAM_A_MAIN_BG);
		for(int i = 0; i < (min_font_bin_size / 2); i++) {
			((volatile uint16_t*)0x06000000)[i] = min_font_bin[i * 2] | (min_font_bin[(i * 2) + 1] << 8);
		}
	}

	// Reset into a passme loop
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	
	*(vu32*)0x02FFFFFC = 0;

	// Return to passme loop
	*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	*(vu32*)0x02FFFE24 = (u32)0x02FFFE04;  // Set ARM9 Loop address --> resetARM9(0x02FFFE04);

	// Reset ARM7
	resetARM7(load_data.copy_location);

	// swi soft reset
	swiSoftReset();
}

