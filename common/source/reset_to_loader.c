#include <nds/bios.h>

#include <string.h>
#include <stddef.h>
#include "reset_to_loader.h"

#include "sr_data_srllastran.h"	// For rebooting the game with unlaunch

uint8_t* getSDResetData() {
	return sr_data_srllastran;
}

static void unlaunchSetFilename(uint8_t* path, size_t max_size) {
	memset((uint8_t*)0x02000800, 0, 0x400);
	memcpy((uint8_t*)0x02000800, "AutoLoadInfo", 12);
	*(uint16_t*)(0x0200080C) = 0x3F0;		// Unlaunch Length for CRC16 (fixed, must be 3F0h)
	*(uint32_t*)(0x02000810) = (BIT(0) | BIT(1));		// Load the title at 2000838h
													// Use colors 2000814h
	*(uint16_t*)(0x02000814) = 0x7FFF;		// Unlaunch Upper screen BG color (0..7FFFh)
	*(uint16_t*)(0x02000816) = 0x7FFF;		// Unlaunch Lower screen BG color (0..7FFFh)

	uint16_t* out_path = (uint16_t*)0x02000838;	// Unlaunch Device:/Path/Filename.ext (16bit Unicode,end by 0000h)
	if(memcmp(path, "sd:/", 4) == 0) {	// Unlaunch wants sd as sdmc...
		out_path[0] = 's';
		out_path[1] = 'd';
		out_path[2] = 'm';
		out_path[3] = 'c';
		out_path[4] = ':';
		out_path[5] = '/';
		out_path += 6;
		path += 4;
	}

	for(size_t i = 0; i < max_size; i++)
		out_path[i] = path[i];			// Convert ascii to 16 bit Unicode

	*(uint16_t*)(0x0200080E) = swiCRC16(0xFFFF, (void*)0x02000810, 0x3F0);		// Unlaunch CRC16
}

void setRebootParams(struct cardengine_main_data_t* cardengine_main_data, uint8_t* boot_path, uint8_t* reset_data) {
	if(cardengine_main_data->boot_type == CARDENGINE_BOOT_TYPE_SD) {
		if(boot_path[0] == '\0')
			cardengine_main_data->boot_type = CARDENGINE_BOOT_TYPE_NOT_SUPPORTED;
		else
			unlaunchSetFilename(boot_path, cardengine_main_data->boot_path_max_len);
	}
	uint16_t* auto_params = (uint16_t*)0x02000000;
	memset(auto_params, 0, 0x400);
	auto_params[0] = BIT(3);
	uint16_t calcCRC = swiCRC16(0xFFFF, auto_params, 0x300);
	auto_params[0x10 / sizeof(auto_params[0])] = calcCRC;
	uint16_t* auto_load_data = (uint16_t*)0x02000300;
	if(cardengine_main_data->boot_type == CARDENGINE_BOOT_TYPE_SD)
		memcpy(auto_load_data, sr_data_srllastran, sizeof(sr_data_srllastran));
	else if(cardengine_main_data->boot_type == CARDENGINE_BOOT_TYPE_DSIWARE)
		memcpy(auto_load_data, reset_data, cardengine_main_data->reset_data_len);
	calcCRC = swiCRC16(0xFFFF, auto_load_data + (0x8 / sizeof(auto_load_data[0])), 0x18);
	if((cardengine_main_data->boot_type != CARDENGINE_BOOT_TYPE_SD) && (cardengine_main_data->boot_type != CARDENGINE_BOOT_TYPE_DSIWARE))
		calcCRC += 1; // Invalidate this, so the main launcher is booted up instead
	auto_load_data[0x6 / sizeof(auto_load_data[0])] = calcCRC;
}
