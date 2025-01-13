/*
	Simple DS Slot-1 Launcher
	Copyright (C) 2024  Lorenzooone

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

#include <nds.h>
#include <nds/fifocommon.h>

#include <stdio.h>
#include <string.h>

#include "nds_card.h"
#include "launch_engine.h"

#define PRINT_FUNCTION printf

struct {
sNDSHeader header;
char padding[0x200 - sizeof(sNDSHeader)];
} ndsHeader;

void cart_reset() {
	if(isDSiMode()) {
		disableSlot1();
		for(int i = 0; i < 10; i++)
			swiWaitForVBlank();
		enableSlot1();
	}
}

void print_data() {
	consoleClear();
	int ram_size = 4;
	if(isDSiMode()) {
		PRINT_FUNCTION ("DSi - ");
		ram_size = 16;
	}
	else
		PRINT_FUNCTION ("DS - ");
	if(isHwDebugger())
		ram_size *= 2;
	
	PRINT_FUNCTION ("%dMB RAM\n", ram_size);
	PRINT_FUNCTION ("Please insert a DS cartridge.\n");
	PRINT_FUNCTION ("Press A to update information.\n");
	PRINT_FUNCTION ("Press START to try to launch the game.\n\n");
	cart_reset();
	cardReadHeader((uint8*)&ndsHeader);
	bool success = true;
	if(*(u32*)&ndsHeader == 0xffffffff)
		success = false;
	if(ndsHeader.header.headerCRC16 != swiCRC16(0xFFFF, (void*)&ndsHeader, 0x15E))
		success = false;
	char name[256];
	sprintf(&name[0], "----");
	if(success) {
		memcpy(&name[0], &ndsHeader.header.gameCode[0], 4);
		name[4] = 0x00;
	}
	PRINT_FUNCTION("ID: %s\n", name);
	sprintf(&name[0], "------------");
	if(success) {
		memcpy(&name[0], &ndsHeader.header.gameTitle[0], 12);
		name[12] = 0x00;
	}
	PRINT_FUNCTION("Name: %s\n", name);
}

bool is_card_ready(bool do_read) {
	if(do_read) {
		cart_reset();
		cardReadHeader((uint8*)&ndsHeader);
	}
	// Wait for card to stablize before continuing
	for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
	// Check header CRC
	return ndsHeader.header.headerCRC16 == swiCRC16(0xFFFF, (void*)&ndsHeader, 0x15E);
}

int main() {
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);

	// Reset cheat data, since we won't be using it regardless...
	memset((void*)0x023F0000, 0, 0x8000);
	u32 curr_keys = 0;
	do {
		swiWaitForVBlank();
		scanKeys();
		curr_keys = keysCurrent();
	} while((curr_keys & (KEY_DEBUG | KEY_SELECT)) && (!(curr_keys & KEY_B)));
	bool done = false;

	sysSetCardOwner (BUS_OWNER_ARM9);
	if(!(curr_keys & KEY_B))
		done = is_card_ready(true);

	if(!done) {
		videoSetMode(MODE_0_2D);
		consoleDemoInit();
		print_data();
	}
	while(!done) {
		do {
			swiWaitForVBlank();
			scanKeys();
			curr_keys = keysDown();
		} while(!(curr_keys & ( KEY_A | KEY_START)));

		print_data();

		if(curr_keys & KEY_START)
			done = is_card_ready(false);
	}

	while (1) {
		const auto& gameCode = ndsHeader.header.gameCode;
		runLaunchEngine ((memcmp(gameCode, "UBRP", 4) == 0 || memcmp(gameCode, "AMFE", 4) == 0 || memcmp(gameCode, "ALXX", 4) == 0 || memcmp(ndsHeader.header.gameTitle, "D!S!XTREME", 10) == 0), (memcmp(gameCode, "UBRP", 4) == 0));
	}
	return 0;
}

