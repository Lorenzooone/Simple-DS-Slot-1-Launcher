/*
 main.arm7.c
 
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

#ifndef ARM7
# define ARM7
#endif
#include <nds/ndstypes.h>
#include <nds/arm7/codec.h>
#include <nds/debug.h>
#include <nds/system.h>
#include <nds/interrupts.h>
#include <nds/timers.h>
#include <nds/dma.h>
#include <nds/arm7/audio.h>
#include <nds/ipc.h>
#include <nds/arm7/i2c.h>
#include <string.h>

// #include <nds/registers_alt.h>
// #include <nds/memory.h>
// #include <nds/card.h>
// #include <stdio.h>

#ifndef NULL
#define NULL 0
#endif

#define REG_GPIO_WIFI *(vu16*)0x4004C04

#include "common.h"
#include "tonccpy.h"
#include "read_card.h"
#include "module_params.h"
#include "hook.h"
#include "find.h"
#include "restart_self.h"

#include "bootloader_defs.h"
#include "cardengine_defs.h"
#include "reset_to_loader.h"

extern bool __dsimode;
extern u32 _start;

struct bootloader_main_data_t* bootloader_data = (struct bootloader_main_data_t*)&_start;

extern bool arm9_runCardEngine;

u16 scfgRomBak = 0;

bool my_isDSiMode() {
	return ((vu8)scfgRomBak == 1);
}

bool useTwlCfg = false;
int twlCfgLang = 0;

extern void arm7_clearmem (void* loc, size_t len);
extern __attribute__((noreturn)) void arm7_actual_jump(void* fn);
extern void ensureBinaryDecompressed(const tNDSHeader* ndsHeader, module_params_t* moduleParams);

#define CHEAT_DATA_SIZE 0x8000

#define NUM_ELEMS(array) (sizeof(array) / sizeof(array[0]))

#define CHEAT_DATA_END_SIGNATURE_FIRST 0xCF000000

#define MODULE_PARAMS_PERSONAL_SIZE 3
#define MODULE_PARAMS_SIGNATURE_SIZE 2
// Module params - Add start to avoid being mistaken for using old SDK version
static const u32 moduleParamsPersonal[MODULE_PARAMS_PERSONAL_SIZE] = {0x0503757C, 0xDEC00621, 0x2106C0DE};
static module_params_t emulatedModuleParams;

#define SLEEP_INPUT_WRITE_END_SIG_SIZE 2
#define SLEEP_INPUT_WRITE_END_SIG_SDK_POS 1
// Sleep input write
static const u32 sleepInputWriteEndSignatureEndBase[SLEEP_INPUT_WRITE_END_SIG_SIZE] = {0x04000136, 0xFA8};
static const u32 sleepInputWriteSignature[1] = {0x13A04902}; // movne r4, 0x8000
static const u32 sleepInputWriteSignature2[1] = {0x11A05004}; // movne r5, r4
static const u16 sleepInputWriteBeqSignatureThumb[1] = {0xD000};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Important things
#define NDS_HEAD 0x027FFE00
tNDSHeader* ndsHeader = (tNDSHeader*)NDS_HEAD;

#define DSI_INFO_BASE_ADDR		0x027FF000
#define DSI_INFO_BASE_ADDR_SDK5	0x02FFF000

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Used for debugging purposes
/* Disabled for now. Re-enable to debug problems
static void errorOutput (u32 code) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);
	// Set the error code, then tell ARM9 to display it
	arm9_errorCode = code;
	arm9_errorClearBG = true;
	arm9_stateFlag = ARM9_DISPERR;
	// Stop
	while (1);
}
*/

static void debugOutput (u32 code) {
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);
	// Set the error code, then tell ARM9 to display it
	arm9_errorCode = code;
	arm9_errorClearBG = false;
	arm9_stateFlag = ARM9_DISPERR;
	// Wait for completion
	while (arm9_stateFlag != ARM9_READY);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Game information

// Implicit SDK 5
static bool ROMsupportsDsiMode(const tNDSHeader* ndsHeader) {
	return (ndsHeader->unitCode > 0);
}

u32* findModuleParamsOffset(const tNDSHeader* ndsHeader) {
	//dbg_printf("findModuleParamsOffset:\n");

	u32* moduleParamsOffset = findOffset(
			(u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize,
			moduleParamsPersonal + (MODULE_PARAMS_PERSONAL_SIZE - MODULE_PARAMS_SIGNATURE_SIZE), MODULE_PARAMS_SIGNATURE_SIZE 
		);

	// Return NULL if nothing is found
	if(moduleParamsOffset == NULL) {
		if (memcmp(ndsHeader->gameCode, "AS2E", 4) == 0) // Spider-Man 2 (USA)
		{
			emulatedModuleParams.sdk_version = LAST_NON_SDK5_VERSION;
			return (u32*)&emulatedModuleParams;
		}
		// This ROM is implicitly SDK5, as DSi support needs >= SDK5...
		if(ROMsupportsDsiMode(ndsHeader)) {
			emulatedModuleParams.sdk_version = FIRST_SDK5_VERSION;
			return (u32*)&emulatedModuleParams;
		}

		return NULL;
	}

	uintptr_t subtract_value = sizeof(module_params_t) - (sizeof(u32) * MODULE_PARAMS_SIGNATURE_SIZE);
	uintptr_t base_ptr = (uintptr_t)moduleParamsOffset;

	// This would be a really weird case. Return NULL
	if(base_ptr < subtract_value)
		return NULL;

	return (u32*)(base_ptr - subtract_value);
}

void* findSleepInputWriteOffset(const tNDSHeader* ndsHeader, bool* isArm) {
	const module_params_t* moduleParams = (const module_params_t*)findModuleParamsOffset(ndsHeader);

	// dbg_printf("findSleepInputWriteOffset:\n");
	u32 own_sleep_signature[SLEEP_INPUT_WRITE_END_SIG_SIZE] = {0};

	for(int i = 0; i < SLEEP_INPUT_WRITE_END_SIG_SIZE; i++)
		own_sleep_signature[i] = sleepInputWriteEndSignatureEndBase[i];
	own_sleep_signature[SLEEP_INPUT_WRITE_END_SIG_SDK_POS] += isSdk5(moduleParams) ? DSI_INFO_BASE_ADDR_SDK5 : DSI_INFO_BASE_ADDR;

	u32* endOffset = findOffset(
		(u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize,
		own_sleep_signature, NUM_ELEMS(own_sleep_signature)
	);

	if(!endOffset)
		return NULL;

	u32* armOffset = findOffsetBackwards(
		endOffset, 0x38,
		sleepInputWriteSignature, NUM_ELEMS(sleepInputWriteSignature)
	);
	if(armOffset) {
		*isArm = true;
		return armOffset;
	}

	armOffset = findOffsetBackwards(
		endOffset, 0x3C,
		sleepInputWriteSignature2, NUM_ELEMS(sleepInputWriteSignature2)
	);
	if(armOffset) {
		*isArm = true;
		return armOffset;
	}

	u16* thumbOffset = (u16*)findOffsetBackwardsThumb(
		(u16*)endOffset, 0x30,
		sleepInputWriteBeqSignatureThumb, NUM_ELEMS(sleepInputWriteBeqSignatureThumb)
	);
	if(thumbOffset) {
		*isArm = false;
		return thumbOffset + 1;
	}

	return NULL;
}

static void patchSleepInputWrite(const tNDSHeader* ndsHeader) {
	if (bootloader_data->sleepMode) {
		return;
	}

	bool isArm = true;
	u32* offset = (u32*)findSleepInputWriteOffset(ndsHeader, &isArm);
	if (!offset) {
		return;
	}

	if(isArm) {
		*offset = 0xE1A00000; // nop
	} else {
		u16* offsetThumb = (u16*)offset;
		*offsetThumb = 0x46C0; // nop
	}

}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Firmware stuff

#define FW_READ        0x03

void arm7_readFirmware (uint32 address, uint8 * buffer, uint32 size) {
  uint32 index;

  // Read command
  while (REG_SPICNT & SPI_BUSY);
  REG_SPICNT = SPI_ENABLE | SPI_CONTINUOUS | SPI_DEVICE_NVRAM;
  REG_SPIDATA = FW_READ;
  while (REG_SPICNT & SPI_BUSY);

  // Set the address
  REG_SPIDATA =  (address>>16) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address>>8) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);
  REG_SPIDATA =  (address) & 0xFF;
  while (REG_SPICNT & SPI_BUSY);

  for (index = 0; index < size; index++) {
    REG_SPIDATA = 0;
    while (REG_SPICNT & SPI_BUSY);
    buffer[index] = REG_SPIDATA & 0xFF;
  }
  REG_SPICNT = 0;
}

/*-------------------------------------------------------------------------
arm7_resetMemory
Clears all of the NDS's RAM that is visible to the ARM7
Written by Darkain.
Modified by Chishm:
 * Added STMIA clear mem loop
--------------------------------------------------------------------------*/
void arm7_resetMemory (void)
{
	int i;
	u8 settings1, settings2;
	u32 settingsOffset = 0;
	
	REG_IME = 0;

	for (i=0; i<16; i++) {
		SCHANNEL_CR(i) = 0;
		SCHANNEL_TIMER(i) = 0;
		SCHANNEL_SOURCE(i) = 0;
		SCHANNEL_LENGTH(i) = 0;
	}
	REG_SOUNDCNT = 0;

	// Clear out ARM7 DMA channels and timers
	for (i=0; i<4; i++) {
		DMA_CR(i) = 0;
		DMA_SRC(i) = 0;
		DMA_DEST(i) = 0;
		TIMER_CR(i) = 0;
		TIMER_DATA(i) = 0;
	}

	REG_RCNT = 0;

	// Clear out FIFO
	REG_IPC_SYNC = 0;
	REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
	REG_IPC_FIFO_CR = 0;

	// clear IWRAM - 037F:8000 to 0380:FFFF, total 96KiB
	arm7_clearmem ((void*)0x037F8000, 96*1024);
	
	// clear most of EXRAM - except after 0x023F0000, which has the cheat data and ARM9 code
	arm7_clearmem ((void*)0x02000000, 0x003F0000);

	// clear last part of EXRAM, skipping the ARM9's section
	arm7_clearmem ((void*)0x023FE000, 0x2000);

	if(my_isDSiMode() || swiIsDebugger())
		arm7_clearmem((void*)0x02400000, 0x400000); // Clear the rest of EXRAM

	REG_IE = 0;
	REG_IF = ~0;
	(*(vu32*)(0x04000000-4)) = 0;  //IRQ_HANDLER ARM7 version
	(*(vu32*)(0x04000000-8)) = ~0; //VBLANK_INTR_WAIT_FLAGS, ARM7 version
	REG_POWERCNT = 1;  //turn off power to stuffs
	
	// Get settings location
	arm7_readFirmware((u32)0x00020, (u8*)&settingsOffset, 0x2);
	settingsOffset *= 8;

	// Reload DS Firmware settings
	arm7_readFirmware(settingsOffset + 0x070, &settings1, 0x1);
	arm7_readFirmware(settingsOffset + 0x170, &settings2, 0x1);
	
	if ((settings1 & 0x7F) == ((settings2+1) & 0x7F)) {
		arm7_readFirmware(settingsOffset + 0x000, (u8*)0x027FFC80, 0x70);
	} else {
		arm7_readFirmware(settingsOffset + 0x100, (u8*)0x027FFC80, 0x70);
	}
	if (bootloader_data->language >= 0 && bootloader_data->language < 6) {
		*(u8*)(0x027FFCE4) = bootloader_data->language;	// Change language
	}
	
	// Load FW header 
	arm7_readFirmware((u32)0x000000, (u8*)0x027FF830, 0x20);
}

static void NDSTouchscreenMode(void) {
	//unsigned char * *(unsigned char*)0x40001C0=		(unsigned char*)0x40001C0;
	//unsigned char * *(unsigned char*)0x40001C0byte2=(unsigned char*)0x40001C1;
	//unsigned char * *(unsigned char*)0x40001C2=	(unsigned char*)0x40001C2;
	//unsigned char * I2C_DATA=	(unsigned char*)0x4004500;
	//unsigned char * I2C_CNT=	(unsigned char*)0x4004501;

	u8 volLevel;
	
	//if (fifoCheckValue32(FIFO_MAXMOD)) {
	//	// special setting (when found special gamecode)
	//	volLevel = 0xAC;
	//} else {
		// normal setting (for any other gamecodes)
		volLevel = 0xA7;
	//}

	// Touchscreen
	cdcReadReg (0x63, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x00);
	cdcReadReg (CDC_CONTROL, 0x51);
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcReadReg (CDC_CONTROL, 0x3F);
	cdcReadReg (CDC_SOUND, 0x28);
	cdcReadReg (CDC_SOUND, 0x2A);
	cdcReadReg (CDC_SOUND, 0x2E);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x40, 0x0C);
	cdcWriteReg(CDC_SOUND, 0x24, 0xFF);
	cdcWriteReg(CDC_SOUND, 0x25, 0xFF);
	cdcWriteReg(CDC_SOUND, 0x26, 0x7F);
	cdcWriteReg(CDC_SOUND, 0x27, 0x7F);
	cdcWriteReg(CDC_SOUND, 0x28, 0x4A);
	cdcWriteReg(CDC_SOUND, 0x29, 0x4A);
	cdcWriteReg(CDC_SOUND, 0x2A, 0x10);
	cdcWriteReg(CDC_SOUND, 0x2B, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(CDC_SOUND, 0x23, 0x00);
	cdcWriteReg(CDC_SOUND, 0x1F, 0x14);
	cdcWriteReg(CDC_SOUND, 0x20, 0x14);
	cdcWriteReg(CDC_CONTROL, 0x3F, 0x00);
	cdcReadReg (CDC_CONTROL, 0x0B);
	cdcWriteReg(CDC_CONTROL, 0x05, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x0B, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x0C, 0x02);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x02);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x60);
	cdcWriteReg(CDC_CONTROL, 0x01, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x39, 0x66);
	cdcReadReg (CDC_SOUND, 0x20);
	cdcWriteReg(CDC_SOUND, 0x20, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x04, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x81);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x82);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x82);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x04, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x05, 0xA1);
	cdcWriteReg(CDC_CONTROL, 0x06, 0x15);
	cdcWriteReg(CDC_CONTROL, 0x0B, 0x87);
	cdcWriteReg(CDC_CONTROL, 0x0C, 0x83);
	cdcWriteReg(CDC_CONTROL, 0x12, 0x87);
	cdcWriteReg(CDC_CONTROL, 0x13, 0x83);
	cdcReadReg (CDC_TOUCHCNT, 0x10);
	cdcWriteReg(CDC_TOUCHCNT, 0x10, 0x08);
	cdcWriteReg(0x04, 0x08, 0x7F);
	cdcWriteReg(0x04, 0x09, 0xE1);
	cdcWriteReg(0x04, 0x0A, 0x80);
	cdcWriteReg(0x04, 0x0B, 0x1F);
	cdcWriteReg(0x04, 0x0C, 0x7F);
	cdcWriteReg(0x04, 0x0D, 0xC1);
	cdcWriteReg(CDC_CONTROL, 0x41, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x42, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x00);
	cdcWriteReg(0x04, 0x08, 0x7F);
	cdcWriteReg(0x04, 0x09, 0xE1);
	cdcWriteReg(0x04, 0x0A, 0x80);
	cdcWriteReg(0x04, 0x0B, 0x1F);
	cdcWriteReg(0x04, 0x0C, 0x7F);
	cdcWriteReg(0x04, 0x0D, 0xC1);
	cdcWriteReg(CDC_SOUND, 0x2F, 0x2B);
	cdcWriteReg(CDC_SOUND, 0x30, 0x40);
	cdcWriteReg(CDC_SOUND, 0x31, 0x40);
	cdcWriteReg(CDC_SOUND, 0x32, 0x60);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x02);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x10);
	cdcReadReg (CDC_CONTROL, 0x74);
	cdcWriteReg(CDC_CONTROL, 0x74, 0x40);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcReadReg (CDC_CONTROL, 0x51);
	cdcReadReg (CDC_CONTROL, 0x3F);
	cdcWriteReg(CDC_CONTROL, 0x3F, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x23, 0x44);
	cdcWriteReg(CDC_SOUND, 0x1F, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x28, 0x4E);
	cdcWriteReg(CDC_SOUND, 0x29, 0x4E);
	cdcWriteReg(CDC_SOUND, 0x24, 0x9E);
	cdcWriteReg(CDC_SOUND, 0x25, 0x9E);
	cdcWriteReg(CDC_SOUND, 0x20, 0xD4);
	cdcWriteReg(CDC_SOUND, 0x2A, 0x14);
	cdcWriteReg(CDC_SOUND, 0x2B, 0x14);
	cdcWriteReg(CDC_SOUND, 0x26, 0xA7);
	cdcWriteReg(CDC_SOUND, 0x27, 0xA7);
	cdcWriteReg(CDC_CONTROL, 0x40, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x3A, 0x60);
	cdcWriteReg(CDC_SOUND, 0x26, volLevel);
	cdcWriteReg(CDC_SOUND, 0x27, volLevel);
	cdcWriteReg(CDC_SOUND, 0x2E, 0x03);
	cdcWriteReg(CDC_TOUCHCNT, 0x03, 0x00);
	cdcWriteReg(CDC_SOUND, 0x21, 0x20);
	cdcWriteReg(CDC_SOUND, 0x22, 0xF0);
	cdcReadReg (CDC_SOUND, 0x22);
	cdcWriteReg(CDC_SOUND, 0x22, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x52, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x51, 0x00);
	
	// Set remaining values
	cdcWriteReg(CDC_CONTROL, 0x03, 0x44);
	cdcWriteReg(CDC_CONTROL, 0x0D, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x0E, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x0F, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x10, 0x08);
	cdcWriteReg(CDC_CONTROL, 0x14, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x15, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x16, 0x04);
	cdcWriteReg(CDC_CONTROL, 0x1A, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x1E, 0x01);
	cdcWriteReg(CDC_CONTROL, 0x24, 0x80);
	cdcWriteReg(CDC_CONTROL, 0x33, 0x34);
	cdcWriteReg(CDC_CONTROL, 0x34, 0x32);
	cdcWriteReg(CDC_CONTROL, 0x35, 0x12);
	cdcWriteReg(CDC_CONTROL, 0x36, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x37, 0x02);
	cdcWriteReg(CDC_CONTROL, 0x38, 0x03);
	cdcWriteReg(CDC_CONTROL, 0x3C, 0x19);
	cdcWriteReg(CDC_CONTROL, 0x3D, 0x05);
	cdcWriteReg(CDC_CONTROL, 0x44, 0x0F);
	cdcWriteReg(CDC_CONTROL, 0x45, 0x38);
	cdcWriteReg(CDC_CONTROL, 0x49, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x4A, 0x00);
	cdcWriteReg(CDC_CONTROL, 0x4B, 0xEE);
	cdcWriteReg(CDC_CONTROL, 0x4C, 0x10);
	cdcWriteReg(CDC_CONTROL, 0x4D, 0xD8);
	cdcWriteReg(CDC_CONTROL, 0x4E, 0x7E);
	cdcWriteReg(CDC_CONTROL, 0x4F, 0xE3);
	cdcWriteReg(CDC_CONTROL, 0x58, 0x7F);
	cdcWriteReg(CDC_CONTROL, 0x74, 0xD2);
	cdcWriteReg(CDC_CONTROL, 0x75, 0x2C);
	cdcWriteReg(CDC_SOUND, 0x22, 0x70);
	cdcWriteReg(CDC_SOUND, 0x2C, 0x20);

	// Finish up!
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x98);
	cdcWriteReg(0xFF, 0x05, 0x00); //writeTSC(0x00, 0xFF);

	// Power management
	writePowerManagement(PM_READ_REGISTER, 0x00); //*(unsigned char*)0x40001C2 = 0x80, 0x00; // read PWR[0]   ;<-- also part of TSC !
	writePowerManagement(PM_CONTROL_REG, 0x0D); //*(unsigned char*)0x40001C2 = 0x00, 0x0D; // PWR[0]=0Dh    ;<-- also part of TSC !
}

static void DSiTouchscreenMode(void) {
	// Touchscreen
	cdcWriteReg(0, 0x01, 0x01);
	cdcWriteReg(0, 0x39, 0x66);
	cdcWriteReg(1, 0x20, 0x16);
	cdcWriteReg(0, 0x04, 0x00);
	cdcWriteReg(0, 0x12, 0x81);
	cdcWriteReg(0, 0x13, 0x82);
	cdcWriteReg(0, 0x51, 0x82);
	cdcWriteReg(0, 0x51, 0x00);
	cdcWriteReg(0, 0x04, 0x03);
	cdcWriteReg(0, 0x05, 0xA1);
	cdcWriteReg(0, 0x06, 0x15);
	cdcWriteReg(0, 0x0B, 0x87);
	cdcWriteReg(0, 0x0C, 0x83);
	cdcWriteReg(0, 0x12, 0x87);
	cdcWriteReg(0, 0x13, 0x83);
	cdcWriteReg(3, 0x10, 0x88);
	cdcWriteReg(4, 0x08, 0x7F);
	cdcWriteReg(4, 0x09, 0xE1);
	cdcWriteReg(4, 0x0A, 0x80);
	cdcWriteReg(4, 0x0B, 0x1F);
	cdcWriteReg(4, 0x0C, 0x7F);
	cdcWriteReg(4, 0x0D, 0xC1);
	cdcWriteReg(0, 0x41, 0x08);
	cdcWriteReg(0, 0x42, 0x08);
	cdcWriteReg(0, 0x3A, 0x00);
	cdcWriteReg(4, 0x08, 0x7F);
	cdcWriteReg(4, 0x09, 0xE1);
	cdcWriteReg(4, 0x0A, 0x80);
	cdcWriteReg(4, 0x0B, 0x1F);
	cdcWriteReg(4, 0x0C, 0x7F);
	cdcWriteReg(4, 0x0D, 0xC1);
	cdcWriteReg(1, 0x2F, 0x2B);
	cdcWriteReg(1, 0x30, 0x40);
	cdcWriteReg(1, 0x31, 0x40);
	cdcWriteReg(1, 0x32, 0x60);
	cdcWriteReg(0, 0x74, 0x82);
	cdcWriteReg(0, 0x74, 0x92);
	cdcWriteReg(0, 0x74, 0xD2);
	cdcWriteReg(1, 0x21, 0x20);
	cdcWriteReg(1, 0x22, 0xF0);
	cdcWriteReg(0, 0x3F, 0xD4);
	cdcWriteReg(1, 0x23, 0x44);
	cdcWriteReg(1, 0x1F, 0xD4);
	cdcWriteReg(1, 0x28, 0x4E);
	cdcWriteReg(1, 0x29, 0x4E);
	cdcWriteReg(1, 0x24, 0x9E);
	cdcWriteReg(1, 0x25, 0x9E);
	cdcWriteReg(1, 0x20, 0xD4);
	cdcWriteReg(1, 0x2A, 0x14);
	cdcWriteReg(1, 0x2B, 0x14);
	cdcWriteReg(1, 0x26, 0xA7);
	cdcWriteReg(1, 0x27, 0xA7);
	cdcWriteReg(0, 0x40, 0x00);
	cdcWriteReg(0, 0x3A, 0x60);

	// Finish up!
	cdcReadReg (CDC_TOUCHCNT, 0x02);
	cdcWriteReg(CDC_TOUCHCNT, 0x02, 0x00);
}

int arm7_loadBinary (void) {
	u32 chipID;
	u32 errorCode;
	
	// Init card
	errorCode = cardInit(ndsHeader, &chipID);
	if (errorCode) {
		return errorCode;
	}

	// Fix Pokemon games needing header data.
	copyLoop((u32*)0x027FF000, (u32*)NDS_HEAD, 0x170);

	char* romTid = (char*)0x027FF00C;
	if (
		memcmp(romTid, "ADA", 3) == 0    // Diamond
		|| memcmp(romTid, "APA", 3) == 0 // Pearl
		|| memcmp(romTid, "CPU", 3) == 0 // Platinum
		|| memcmp(romTid, "IPK", 3) == 0 // HG
		|| memcmp(romTid, "IPG", 3) == 0 // SS
	) {
		// Make the Pokemon game code ADAJ.
		const char gameCodePokemon[] = { 'A', 'D', 'A', 'J' };
		copyLoop((u32*)0x027FF00C, (u32*)gameCodePokemon, 4);
	}

	// Set memory values expected by loaded NDS
	// from NitroHax, thanks to Chism
	*(u32*)(0x027ff800) = chipID;					// CurrentCardID
	*(u32*)(0x027ff804) = chipID;					// Command10CardID
	*(u16*)(0x027ff808) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
	*(u16*)(0x027ff80a) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	// Copies of above
	*(u32*)(0x027ffc00) = chipID;					// CurrentCardID
	*(u32*)(0x027ffc04) = chipID;					// Command10CardID
	*(u16*)(0x027ffc08) = ndsHeader->headerCRC16;	// Header Checksum, CRC-16 of [000h-15Dh]
	*(u16*)(0x027ffc0a) = ndsHeader->secureCRC16;	// Secure Area Checksum, CRC-16 of [ [20h]..7FFFh]

	*(u16*)(0x027ffc40) = 0x1;						// Boot Indicator -- EXTREMELY IMPORTANT!!! Thanks to cReDiAr
	
	cardReadDirect(ndsHeader->arm9romOffset, (u32*)ndsHeader->arm9destination, ndsHeader->arm9binarySize);
	//ensureBinaryDecompressed(ndsHeader, (module_params_t*) findModuleParamsOffset(ndsHeader));
	cardReadDirect(ndsHeader->arm7romOffset, (u32*)ndsHeader->arm7destination, ndsHeader->arm7binarySize);
	return ERR_NONE;
}

static void restart_data_setup() {
	struct cardengine_main_data_t* cardengine_data = (struct cardengine_main_data_t*)bootloader_data->cardEngineLocation;
	uint8_t instantiated_restart_data[sizeof(restart_self_data)];
	if(cardengine_data->boot_type == CARDENGINE_BOOT_TYPE_SD)
		copyLoop(instantiated_restart_data, getSDResetData(), sizeof(restart_self_data));
	else {
		copyLoop(instantiated_restart_data, restart_self_data, sizeof(restart_self_data));
		copyLoop(instantiated_restart_data + 8, bootloader_data->selfTitleId, sizeof(bootloader_data->selfTitleId));
		copyLoop(instantiated_restart_data + 16, bootloader_data->selfTitleId, sizeof(bootloader_data->selfTitleId));
	}

	if(my_isDSiMode()) {
		if(bootloader_data->runCardEngine) {
			cardengine_data = (struct cardengine_main_data_t*)cardengine_data->copy_location;
			copyLoop(((u8*)cardengine_data) + cardengine_data->reset_data_offset, instantiated_restart_data, sizeof(instantiated_restart_data));
		}
		else if(bootloader_data->redirectPowerButton) {
			uintptr_t cardengine_after_area = 0x02000400;
			if(cardengine_data->boot_type == CARDENGINE_BOOT_TYPE_SD)
				cardengine_after_area = 0x02000C00;
			// Maybe this check should be more complex... For now, it is fine, I think.
			// Basically, we want to ensure we are not overwriting anything with our data.
			if((((uintptr_t)ndsHeader->arm9destination) >= cardengine_after_area) && (((uintptr_t)ndsHeader->arm7destination) >= cardengine_after_area))
				setRebootParams(cardengine_data, (u8*)(bootloader_data->cardEngineLocation + cardengine_data->boot_path_offset), instantiated_restart_data);
		}
		if(bootloader_data->runCardEngine && bootloader_data->redirectPowerButton) {
			i2cWriteRegister(I2C_PM, I2CREGPM_MMCPWR, 1);		// Have IRQ check for power button press
			i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);	// SDK 5 --> Bootflag = Warmboot/SkipHealthSafety
		}
		else {
			i2cWriteRegister(I2C_PM, I2CREGPM_MMCPWR, 0);		// Have IRQ not check for power button press
			i2cWriteRegister(I2C_PM, I2CREGPM_RESETFLAG, 1);	// SDK 5 --> Bootflag = Warmboot/SkipHealthSafety
		}
	}
}

/*-------------------------------------------------------------------------
arm7_startBinary
Jumps to the ARM7 NDS binary in sync with the display and ARM9
Written by Darkain, modified by Chishm.
--------------------------------------------------------------------------*/
void arm7_startBinary (void)
{
	// Wait until the ARM9 is ready
	while (arm9_stateFlag != ARM9_READY);

	while (REG_VCOUNT!=191);
	while (REG_VCOUNT==191);
	
	// Get the ARM9 to boot
	arm9_stateFlag = ARM9_BOOTBIN;

	while (REG_VCOUNT!=191);
	while (REG_VCOUNT==191);

	// Start ARM7
	arm7_actual_jump(*(void**)(0x27FFE34));
}


void initMBK() {
	// arm7 is master of WRAM-A, arm9 of WRAM-B & C
	REG_MBK9=0x3000000F;

	// WRAM-A fully mapped to arm7
	*((vu32*)REG_MBK1)=0x8D898581; // same as dsiware

	// WRAM-B fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK2)=0x8C888480;
	*((vu32*)REG_MBK3)=0x9C989490;

	// WRAM-C fully mapped to arm9 // inverted order
	*((vu32*)REG_MBK4)=0x8C888480;
	*((vu32*)REG_MBK5)=0x9C989490;

	// WRAM mapped to the 0x3700000 - 0x37FFFFF area 
	// WRAM-A mapped to the 0x3000000 - 0x303FFFF area : 256k
	REG_MBK6=0x00403000;
	// WRAM-B mapped to the 0x3740000 - 0x37BFFFF area : 512k // why? only 256k real memory is there
	REG_MBK7=0x07C03740; // same as dsiware
	// WRAM-C mapped to the 0x3700000 - 0x373FFFF area : 256k
	REG_MBK8=0x07403700; // same as dsiware
}


/*void fixFlashcardForDSiMode(void) {
	if ((memcmp(ndsHeader->gameTitle, "PASS", 4) == 0)
	&& (memcmp(ndsHeader->gameCode, "ASME", 4) == 0))		// CycloDS Evolution
	{
		*(u16*)(0x0200197A) = 0xDF02;	// LZ77UnCompReadByCallbackWrite16bit
		*(u16*)(0x020409FA) = 0xDF02;	// LZ77UnCompReadByCallbackWrite16bit
	}
}*/

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Main function

void arm7_main (void) {
	__dsimode = bootloader_data->dsiMode;

	if((!my_isDSiMode()) && (!swiIsDebugger()))
		bootloader_data->runCardEngine = false;

	if(bootloader_data->cardEngineSize == 0)
		bootloader_data->runCardEngine = false;

	if (bootloader_data->runCardEngine) {
		arm9_runCardEngine = bootloader_data->runCardEngine;
		initMBK();
	}

	int errorCode;

	// Wait for ARM9 to at least start
	while (arm9_stateFlag < ARM9_START);

	debugOutput (ERR_STS_CLR_MEM);
	
	scfgRomBak = REG_SCFG_ROM;

	// Get ARM7 to clear RAM
	arm7_resetMemory();	

	debugOutput (ERR_STS_LOAD_BIN);

	REG_SCFG_ROM = 0x703;	// Not running this prevents (some?) flashcards from running

	// Load the NDS file
	errorCode = arm7_loadBinary();
	if (errorCode) {
		debugOutput(errorCode);
	}
	
	if (my_isDSiMode()) {
		bootloader_data->twlTouch ? DSiTouchscreenMode() : NDSTouchscreenMode();
		*(u16*)0x4000500 = 0x807F;

		REG_GPIO_WIFI |= BIT(8);	// Old NDS-Wifi mode
	}

	//if (!bootloader_data->twlMode) {
	//	fixFlashcardForDSiMode();
	//} else {
	//	REG_SCFG_ROM = 0x703;
		if (!bootloader_data->sdAccess) {
			REG_SCFG_CLK = 0x0180;
			REG_SCFG_EXT = 0x93FBFB06;
		}
		else {
			REG_SCFG_CLK = 0x0181;
		}
	//}

	patchSleepInputWrite(ndsHeader);

	bool gameSoftReset = false;

	if (memcmp((char*)NDS_HEAD+0xC, "NTR", 3) == 0		// Download Play ROMs
	 || memcmp((char*)NDS_HEAD+0xC, "ASM", 3) == 0		// Super Mario 64 DS
	 || memcmp((char*)NDS_HEAD+0xC, "AMC", 3) == 0		// Mario Kart DS
	 || memcmp((char*)NDS_HEAD+0xC, "A2D", 3) == 0		// New Super Mario Bros.
	 || memcmp((char*)NDS_HEAD+0xC, "ARZ", 3) == 0		// Rockman ZX/MegaMan ZX
	 || memcmp((char*)NDS_HEAD+0xC, "AKW", 3) == 0		// Kirby Squeak Squad/Mouse Attack
	 || memcmp((char*)NDS_HEAD+0xC, "YZX", 3) == 0		// Rockman ZX Advent/MegaMan ZX Advent
	 || memcmp((char*)NDS_HEAD+0xC, "B6Z", 3) == 0)	// Rockman Zero Collection/MegaMan Zero Collection
	{
		gameSoftReset = true;
	}

	u32* cheatDataBasePos = (u32*)0x023F0000;
	if(bootloader_data->runCardEngine) {

		u32 wram_a_start = 0x037C0000;
		u32 wram_a_end = 0x037FFFFF;
		u32* cardEnginePos = *((u32**)bootloader_data->cardEngineLocation);
		u32* cheatDataPos = (u32*)((((uintptr_t)cardEnginePos) - CHEAT_DATA_SIZE) & ~0x1F);

		if((((u32)cardEnginePos) >= wram_a_start) && (((u32)cardEnginePos) <= wram_a_end)) {
			// WRAM-A mapped to the 0x37C0000 - 0x37FFFFF area : 256k
			REG_MBK6=0x080037C0;
		}

		copyLoop(cardEnginePos, (u32*)bootloader_data->cardEngineLocation, bootloader_data->cardEngineSize);
		arm7_clearmem(cheatDataPos, CHEAT_DATA_SIZE); // Zero out cheat data
		cheatDataPos[0] = CHEAT_DATA_END_SIGNATURE_FIRST;
		cheatDataPos[CHEAT_DATA_SIZE / sizeof(cheatDataPos[0])] = CHEAT_DATA_END_SIGNATURE_FIRST;

		errorCode = hookNdsRetail(ndsHeader, cardEnginePos, cheatDataPos, gameSoftReset, bootloader_data->language, bootloader_data->redirectPowerButton);
		if (errorCode == ERR_NONE) {
			nocashMessage("card hook Sucessfull");
		} else {
			nocashMessage("error during card hook");
			debugOutput(errorCode);
		}
		if((errorCode == ERR_NONE) && (cheatDataBasePos[0] != CHEAT_DATA_END_SIGNATURE_FIRST))
			copyLoop(cheatDataPos, cheatDataBasePos, CHEAT_DATA_SIZE); // Copy cheat data
	}
	arm7_clearmem(cheatDataBasePos, CHEAT_DATA_SIZE); // Clear cheat data from main memory

	debugOutput (ERR_STS_START);

	if (!bootloader_data->scfgUnlock) {
		// lock SCFG
		REG_SCFG_EXT &= ~(1UL << 31);
	}

	restart_data_setup();

	arm7_startBinary();
}

