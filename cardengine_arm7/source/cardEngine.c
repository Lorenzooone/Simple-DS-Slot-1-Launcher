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

#include <nds/fifomessages.h>
#include <nds/ipc.h>
#include <nds/interrupts.h>
#include <nds/system.h>
#include <nds/input.h>
#include <nds/arm7/audio.h>

#include <string.h>
#include "cardengine_defs.h"
#include "reset_to_loader.h"
#include "cardEngine.h"
#include "i2c.h"

extern void cheat_engine_start(void);

extern u32 copy_offset;
extern u8 reset_data;
extern u8 boot_path;

struct cardengine_main_data_t* cardengine_main_data = (struct cardengine_main_data_t*)&copy_offset;

#ifdef DO_DSI
static int powerOffTimer = 0;
#endif

void myIrqHandlerVBlank(void) {

	if (cardengine_main_data->language >= 0 && cardengine_main_data->language <= 7) {
		// Change language
		*(u8*)(0x027FFCE4) = cardengine_main_data->language;
	}

	#ifdef DO_DSI
	bool wants_soft_reset = (0 == (REG_KEYINPUT & (KEY_L | KEY_R | KEY_START | KEY_SELECT))) && !cardengine_main_data->gameSoftReset;
	// The GBATek docs for this are wrong... :/
	uint8_t power_press_kind = cardengine_main_data->read_power_button ? i2cReadRegister(0x4a, 0x10) : 0;
	bool start_power_press = power_press_kind & 8;
	bool released_power = power_press_kind & 1;
	if(start_power_press || powerOffTimer)
		powerOffTimer++;
	if(released_power)
		powerOffTimer = 0;

	if(powerOffTimer >= 60) {
		systemShutDown();
		// If here, the shutdown failed...
		// Still, try resetting...
		released_power = true;
	}

	wants_soft_reset = wants_soft_reset || released_power;
	if(wants_soft_reset) {
		REG_MASTER_VOLUME = 0;
		int oldIME = enterCriticalSection();
		setRebootParams(cardengine_main_data, &boot_path, &reset_data);
		i2cWriteRegister(0x4a,0x70,0x01);
		i2cWriteRegister(0x4a,0x11,0x01);	// Reboot game
		leaveCriticalSection(oldIME);
		// From blocksds code, 20 ms
		swiDelay(20 * 0x20BA);
	}

	if (REG_IE & IRQ_NETWORK) {
		REG_IE &= ~IRQ_NETWORK; // DSi RTC fix
	}
	#endif

	#ifdef DEBUG
	nocashMessage("cheat_engine_start\n");
	#endif
	
	cheat_engine_start();
}
