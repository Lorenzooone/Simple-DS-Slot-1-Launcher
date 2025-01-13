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
#include "launch_engine.h"
#include "tonccpy.h"
#include "min_font_bin.h"

#define LCDC_BANK_D (u16*)0x06860000

#define DSIMODE_OFFSET 4
#define LANGUAGE_OFFSET 8
#define SDACCESS_OFFSET 12
#define SCFGUNLOCK_OFFSET 16
#define TWLMODE_OFFSET 20
#define TWLCLOCK_OFFSET 24
#define BOOSTVRAM_OFFSET 28
#define TWLTOUCH_OFFSET 32
#define SOUNDFREQ_OFFSET 36
#define RUNCARDENGINE_OFFSET 40

void runLaunchEngine (bool altBootloader, bool isDSBrowser)
{
	bool pass_min_font = true;
	int dsi_mode_enabled = 1;
	int scfgUnlock = 0;
	int sdaccess = 1;
	int twlmode = 1;
	int twlclk = 1;
	int twlvram = 1;
	int twltouch = 1;
	int soundFreq = 0;
	int runCardEngine = 0;
	if(!isDSiMode()) {
		dsi_mode_enabled = 0;
		sdaccess = 0;
		twlmode = 0;
		twlclk = 0;
		twlvram = 0;
		twltouch = 0;
	}
	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM bank D
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;

	// Clear VRAM
	toncset (LCDC_BANK_D, 0, 128 * 1024);

	// Load the loader/patcher into the correct address
	tonccpy (LCDC_BANK_D, altBootloader ? loadAlt_bin : load_bin, altBootloader ? loadAlt_bin_size : load_bin_size);

	// Set the parameters for the loader
	toncset32 ((u8*)LCDC_BANK_D+DSIMODE_OFFSET, dsi_mode_enabled, 1);	// Not working?
	toncset32 ((u8*)LCDC_BANK_D+LANGUAGE_OFFSET, -1, 1);
	toncset32 ((u8*)LCDC_BANK_D+SDACCESS_OFFSET, sdaccess, 1);
	toncset32 ((u8*)LCDC_BANK_D+SCFGUNLOCK_OFFSET, scfgUnlock, 1);
	toncset32 ((u8*)LCDC_BANK_D+TWLMODE_OFFSET, twlmode, 1);
	toncset32 ((u8*)LCDC_BANK_D+TWLCLOCK_OFFSET, twlclk, 1);
	toncset32 ((u8*)LCDC_BANK_D+BOOSTVRAM_OFFSET, twlvram, 1);
	toncset32 ((u8*)LCDC_BANK_D+TWLTOUCH_OFFSET, twltouch, 1);
	toncset32 ((u8*)LCDC_BANK_D+SOUNDFREQ_OFFSET, soundFreq, 1);
	toncset32 ((u8*)LCDC_BANK_D+RUNCARDENGINE_OFFSET, runCardEngine, 1);

	irqDisable(IRQ_ALL);

	if (altBootloader) {
		if (isDSBrowser || scfgUnlock) {
			REG_SCFG_EXT = twlvram ? 0x8300E000 : 0x8300C000;
		} else {
			REG_SCFG_EXT = twlvram ? 0x83002000 : 0x83000000;
		}
		if (!scfgUnlock) {
			REG_SCFG_EXT &= ~(1UL << 31);
		}
	}

	if(pass_min_font) {
		VRAM_A_CR = VRAM_ENABLE;
		vramSetBankA(VRAM_A_MAIN_BG);
		for(int i = 0; i < (min_font_bin_size / 2); i++) {
			((volatile uint16_t*)0x06000000)[i] = min_font_bin[i * 2] | (min_font_bin[(i * 2) + 1] << 8);
		}
	}

	// Give the VRAM to the ARM7
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_ARM7_0x06020000;
	
	// Reset into a passme loop
	REG_EXMEMCNT |= ARM7_OWNS_ROM | ARM7_OWNS_CARD;
	
	*(vu32*)0x02FFFFFC = 0;

	// Return to passme loop
	*(vu32*)0x02FFFE04 = (u32)0xE59FF018; // ldr pc, 0x02FFFE24
	*(vu32*)0x02FFFE24 = (u32)0x02FFFE04;  // Set ARM9 Loop address --> resetARM9(0x02FFFE04);

	// Reset ARM7
	resetARM7(0x06020000);

	// swi soft reset
	swiSoftReset();
}

