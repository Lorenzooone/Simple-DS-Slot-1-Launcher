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

#define LCDC_BANK_D (u16*)0x06860000

#define LANGUAGE_OFFSET 8

void runLaunchEngine (bool altBootloader, bool isDSBrowser)
{
	irqDisable(IRQ_ALL);

	// Direct CPU access to VRAM bank D
	VRAM_D_CR = VRAM_ENABLE | VRAM_D_LCD;

	// Clear VRAM
	toncset (LCDC_BANK_D, 0, 128 * 1024);

	// Load the loader/patcher into the correct address
	tonccpy (LCDC_BANK_D, altBootloader ? loadAlt_bin : load_bin, altBootloader ? loadAlt_bin_size : load_bin_size);

	// Set the parameters for the loader
	toncset32 ((u8*)LCDC_BANK_D+LANGUAGE_OFFSET, -1, 1);

	irqDisable(IRQ_ALL);

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

