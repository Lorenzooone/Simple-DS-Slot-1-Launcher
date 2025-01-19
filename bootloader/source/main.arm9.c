/*
 main.arm9.c
 
 By Michael Chisholm (Chishm)
 
 All resetMemory and startBinary functions are based 
 on the MultiNDS loader by Darkain.
 Original source available at:
 http://cvs.sourceforge.net/viewcvs.py/ndslib/ndslib/examples/loader/boot/main.cpp

 License:
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

#define ARM9
#undef ARM7
#include <nds/memory.h>
#include <nds/arm9/background.h>
#include <nds/arm9/video.h>
#include <nds/arm9/input.h>
#include <nds/interrupts.h>
#include <nds/dma.h>
#include <nds/timers.h>
#include <nds/system.h>
#include <nds/ipc.h>

#include <nds/dma.h>
#include <nds/card.h>
#include <stddef.h> // NULL
#include <stdlib.h>

#include "common.h"
#include "min_font_bin.h"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
#define NDS_HEADER         0x027FFE00
#define NDS_HEADER_SDK5    0x02FFFE00 // __NDSHeader
#define NDS_HEADER_POKEMON 0x027FF000

#define DSI_HEADER         0x027FE000
#define DSI_HEADER_SDK5    0x02FFE000 // __DSiHeader
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

tNDSHeader* ndsHeader;
volatile bool dsiModeConfirmed = false;
volatile bool arm9_dsimode = false;
volatile bool arm9_boostVram = false;
volatile bool arm9_scfgUnlock = false;
volatile bool arm9_extendedMemory = false;
volatile bool arm9_isSdk5 = false;
volatile bool arm9_runCardEngine = false;
volatile int arm9_stateFlag = ARM9_BOOT;
volatile u32 arm9_errorCode = 0;
volatile bool arm9_errorClearBG = false;
volatile u32 arm9_BLANK_RAM = 0;

// For debugging
volatile uint32_t data_saved[4];

/*-------------------------------------------------------------------------
External functions
--------------------------------------------------------------------------*/
extern void arm9_clearCache (void);
extern void arm9_flush_cache(void);

void initMBKARM9() {
	// default dsiware settings

	// WRAM-B fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK2)=0x8C888480;
	*((vu32*)REG_MBK3)=0x9C989490;

	// WRAM-C fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK4)=0x8C888480;
	*((vu32*)REG_MBK5)=0x9C989490;

	// WRAM-A not mapped (reserved to arm7)
	REG_MBK6=0x00000000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}

void initMBKARM9_dsiMode(void) {
	*(vu32*)REG_MBK1 = *(u32*)0x02FFE180;
	*(vu32*)REG_MBK2 = *(u32*)0x02FFE184;
	*(vu32*)REG_MBK3 = *(u32*)0x02FFE188;
	*(vu32*)REG_MBK4 = *(u32*)0x02FFE18C;
	*(vu32*)REG_MBK5 = *(u32*)0x02FFE190;
	REG_MBK6 = *(u32*)0x02FFE194;
	REG_MBK7 = *(u32*)0x02FFE198;
	REG_MBK8 = *(u32*)0x02FFE19C;
	REG_MBK9 = *(u32*)0x02FFE1AC;
}

void SetBrightness(u8 screen, s8 bright) {
	u16 mode = 1 << 14;

	if (bright < 0) {
		mode = 2 << 14;
		bright = -bright;
	}
	if (bright > 31) {
		bright = 31;
	}
	*(vu16*)(0x0400006C + (0x1000 * screen)) = bright + mode;
}

/*-------------------------------------------------------------------------
arm9_errorOutput
Displays an error code on screen.

Each box is 2 bits, and left-to-right is most-significant bits to least.
Red = 00, Yellow = 01, Green = 10, Blue = 11

Written by Chishm
--------------------------------------------------------------------------*/
static void arm9_errorOutput (u32 code, bool clearBG) {
// Re-enable for debugging
	int i, j, k;
	u16 colour;
	
	REG_POWERCNT = (vu16) (POWER_LCD | POWER_2D_A);
	REG_DISPCNT = MODE_FB0;
	VRAM_A_CR = VRAM_ENABLE;
	
	if (clearBG) {
		// Clear display
		for (i = 0; i < 256*192; i++) {
			VRAM_A[i] = 0x0000;
		}
	}
	
	// Draw boxes of colour, signifying error codes
	
	if ((code >> 16) != 0) {
		// high 16 bits
		for (i = 0; i < 8; i++) {						// Pair of bits to use
			if (((code>>(30-2*i))&3) == 0) {
				colour = 0x001F; // Red
			} else if (((code>>(30-2*i))&3) == 1) {
				colour = 0x03FF; // Yellow
			} else if (((code>>(30-2*i))&3) == 2) {
				colour = 0x03E0; // Green
			} else {
				colour = 0x7C00; // Blue
			}
			for (j = 71; j < 87; j++) { 				// Row
				for (k = 32*i+8; k < 32*i+24; k++) {	// Column
					VRAM_A[j*256+k] = colour;
				}
			}
		}
	}

	if ((code >> 8) != 0) {
		// Low 16 bits
		for (i = 0; i < 8; i++) {						// Pair of bits to use
			if (((code>>(14-2*i))&3) == 0) {
				colour = 0x001F; // Red
			} else if (((code>>(14-2*i))&3) == 1) {
				colour = 0x03FF; // Yellow
			} else if (((code>>(14-2*i))&3) == 2) {
				colour = 0x03E0; // Green
			} else {
				colour = 0x7C00; // Blue
			}
			for (j = 103; j < 119; j++) { 				// Row
				for (k = 32*i+8; k < 32*i+24; k++) {	// Column
					VRAM_A[j*256+k] = colour;
				}
			}
		}
	} else {
		// Low 8 bits
		for (i = 0; i < 4; i++) {						// Pair of bits to use
			if (((code>>(6-2*i))&3) == 0) {
				colour = 0x001F; // Red
			} else if (((code>>(6-2*i))&3) == 1) {
				colour = 0x03FF; // Yellow
			} else if (((code>>(6-2*i))&3) == 2) {
				colour = 0x03E0; // Green
			} else {
				colour = 0x7C00; // Blue
			}
			for (j = 87; j < 103; j++) { 				// Row
				for (k = 32*i+72; k < 32*i+88; k++) {	// Column
					VRAM_A[j*256+k] = colour;
				}
			}
		}
	}		
}

static void ux_to_screen(volatile uint16_t* vram_map, uint32_t value, size_t num_displayed_bytes) {
	size_t num_nybbles = 2 * num_displayed_bytes;
	for(int i = 0; i < num_nybbles; i++)
		vram_map[i] = ((value >> ((4 * (num_nybbles - 1 - i)))) & 0xF) + 1;
}

static void u8_to_screen(volatile uint16_t* vram_map, uint8_t value) {
	ux_to_screen(vram_map, value, 1);
}

static void u32_to_screen(volatile uint16_t* vram_map, uint32_t value) {
	ux_to_screen(vram_map, value, 4);
}

void memory_view_to_screen(uint8_t* address) {
	#define TILE_1D_MAP (1<<4)
	#define ACTIVATE_SCREEN_HW (1<<16)
	REG_POWERCNT = (vu16) (POWER_LCD | POWER_2D_A);
	VRAM_A_CR = VRAM_ENABLE;
	REG_DISPSTAT = 0;
    REG_DISPCNT = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW | (0x1 << 8);
    REG_DISPCNT_SUB = 0 | TILE_1D_MAP | ACTIVATE_SCREEN_HW | (0x1 << 8);
	//videoSetMode(0);
	//videoSetModeSub(0);
	BG_PALETTE[0] = 0x7C00;
	BG_PALETTE[1] = 0x7FFF;
	BG_PALETTE[2] = 0x7C00;
	BG_PALETTE_SUB[0] = 0x7C00;
	BG_PALETTE_SUB[1] = 0x7FFF;
	BG_PALETTE_SUB[2] = 0x7C00;
	REG_BG0CNT = 0x0100;
	REG_BG0CNT_SUB = 0x0100;
	vramSetBankA(VRAM_A_MAIN_BG);

	volatile uint16_t* vram_target_map = (volatile uint16_t*)0x06000800;
	size_t single_iter_shown = 8;
	for(int i = 0; i < SCREEN_HEIGHT >> 3; i++) {
		u32_to_screen(&vram_target_map[(i * (SCREEN_WIDTH >> 3))], (uintptr_t)(address + (single_iter_shown * i)));
		int start = 9;
		for(int j = 0; j < single_iter_shown; j++)
			u8_to_screen(&vram_target_map[(i * (SCREEN_WIDTH >> 3)) + start + (j * 3)], address[(single_iter_shown * i) + j]);
	}

	while(REG_KEYINPUT & KEY_A);
	for(int i = 0; i < (SCREEN_HEIGHT >> 3) * (SCREEN_WIDTH >> 3); i++)
		vram_target_map[i] = 0;
}

/*-------------------------------------------------------------------------
arm9_main
Clears the ARM9's icahce and dcache
Clears the ARM9's DMA channels and resets video memory
Jumps to the ARM9 NDS binary in sync with the display and ARM7
Written by Darkain, modified by Chishm
--------------------------------------------------------------------------*/
void __attribute__((target("arm"))) arm9_main (void) {
	register int i;
	
	//set shared ram to ARM7
	WRAM_CR = 0x03;
	REG_EXMEMCNT = 0xE880;

	initMBKARM9();

	arm9_stateFlag = ARM9_START;

	REG_IME = 0;
	REG_IE = 0;
	REG_IF = ~0;

	arm9_clearCache();

	for (i=0; i<16*1024; i+=4) {  //first 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
		(*(vu32*)(i+0x00800000)) = 0x00000000;      //clear DTCM
	}
	
	for (i=16*1024; i<32*1024; i+=4) {  //second 16KB
		(*(vu32*)(i+0x00000000)) = 0x00000000;      //clear ITCM
	}

	arm9_stateFlag = ARM9_MEMCLR;

	(*(vu32*)0x00803FFC) = 0;   //IRQ_HANDLER ARM9 version
	(*(vu32*)0x00803FF8) = ~0;  //VBLANK_INTR_WAIT_FLAGS ARM9 version

	//clear out ARM9 DMA channels
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	bool ignore_min_font = true;
	int vram_a_u16_offset = 0;
	int vram_a_size_removed = 0;
	if(ignore_min_font) {
		vram_a_u16_offset = min_font_bin_size / 2;
		vram_a_size_removed = vram_a_u16_offset * 2;
	}
	
	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	VRAM_A_CR = 0x80;
	VRAM_B_CR = 0x80;
	VRAM_C_CR = 0x80;
// Don't mess with the VRAM used for execution
//	VRAM_D_CR = 0x80;
	VRAM_E_CR = 0x80;
	VRAM_F_CR = 0x80;
	VRAM_G_CR = 0x80;
	VRAM_H_CR = 0x80;
	VRAM_I_CR = 0x80;
	BG_PALETTE[0] = 0xFFFF;
	dmaFill((u16*)&arm9_BLANK_RAM, BG_PALETTE+1, (2*1024)-2);
	dmaFill((u16*)&arm9_BLANK_RAM, OAM, 2*1024);
	dmaFill((u16*)&arm9_BLANK_RAM, (u16*)0x04000000, 0x56);  // Clear main display registers
	dmaFill((u16*)&arm9_BLANK_RAM, (u16*)0x04001000, 0x56);  // Clear sub display registers
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_A + vram_a_u16_offset, (0x20000*3) - vram_a_size_removed);		// Banks A, B, C
	dmaFill((u16*)&arm9_BLANK_RAM, VRAM_D, 272*1024);		// Banks D (excluded), E, F, G, H, I

	REG_DISPSTAT = 0;
	//videoSetMode(0);
	//videoSetModeSub(0);
	VRAM_A_CR = 0;
	VRAM_B_CR = 0;
	VRAM_C_CR = 0;
	// Don't mess with the ARM7's VRAM
	//VRAM_D_CR = 0;
	VRAM_E_CR = 0;
	VRAM_F_CR = 0;
	VRAM_G_CR = 0;
	VRAM_H_CR = 0;
	VRAM_I_CR = 0;
	REG_POWERCNT = 0x820F;

	*(vu16*)0x0400006C |= BIT(14);
	*(vu16*)0x0400006C &= BIT(15);
	SetBrightness(0, 0);
	SetBrightness(1, 0);

	// set ARM9 state to ready and wait for it to change again
	arm9_stateFlag = ARM9_READY;
	while (arm9_stateFlag != ARM9_BOOTBIN) {
		if (arm9_stateFlag == ARM9_DISPERR) {
			// Re-enable for debugging
			arm9_errorOutput (arm9_errorCode, arm9_errorClearBG);
			if (arm9_stateFlag == ARM9_DISPERR) {
				arm9_stateFlag = ARM9_READY;
			}
		}
		if (arm9_stateFlag == ARM9_SETSCFG) {
			if(arm9_dsimode) {
				if(dsiModeConfirmed) {
					if(arm9_isSdk5 && ndsHeader->unitCode > 0) {
						initMBKARM9_dsiMode();
					}
					REG_SCFG_EXT = 0x8307F100;
					REG_SCFG_CLK = 0x87;
					REG_SCFG_RST = 1;
				}
				else {
					REG_SCFG_EXT = (arm9_extendedMemory ? 0x8300C000 : 0x83000000);
					if (arm9_boostVram) {
						REG_SCFG_EXT |= BIT(13);	// Extended VRAM Access
					}
					if (!arm9_scfgUnlock) {
						// lock SCFG
						REG_SCFG_EXT &= ~(1UL << 31);
					}
				}
			}
			arm9_stateFlag = ARM9_READY;
		}
		if(arm9_stateFlag == ARM9_PRINT_MEM) {
			memory_view_to_screen((uint8_t*)0x2003FC0);
			arm9_stateFlag = ARM9_READY;
		}
		arm9_flush_cache();
	}
	arm9_stateFlag = ARM9_READY;
	
	// wait for vblank then boot
	while(REG_VCOUNT!=191);
	while(REG_VCOUNT==191);
	//memory_view_to_screen((uint8_t*)0x02E80000);
	//memory_view_to_screen((uint8_t*)0x02400380);
	//memory_view_to_screen((uint8_t*)0x4004000);
	//memory_view_to_screen((uint8_t*)NDS_HEADER);
	//memory_view_to_screen((uint8_t*)0x2380000);
	//memory_view_to_screen((uint8_t*)0x2380000);
	//memory_view_to_screen((uint8_t*)0x02EC7040);

	// Ensure the reset bit of REG_ROMCTRL is set...
	REG_ROMCTRL |= CARD_nRESET;
	REG_IE = 0;
	REG_IF = ~0;
	VoidFn arm9code = (VoidFn)ndsHeader->arm9executeAddress;
	arm9code();
}

