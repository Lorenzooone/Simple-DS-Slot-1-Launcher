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

#include <nds.h>
#include <nds/arm7/input.h>
#include <nds/system.h>

void VcountHandler() {
	inputGetAndSend();
}

void VblankHandler(void) {
}

// Determine if 3DS or DSi
static bool is_device_3ds() {
	bool is_3ds = false;

	uint8_t read_val = i2cReadRegister(0x4A, 0x80);
	uint8_t to_write_val = 0x10;
	if(read_val == to_write_val)
		to_write_val = 0x11;

	// Check if writing this register works
	i2cWriteRegister(0x4A, 0x80, to_write_val);
	if(i2cReadRegister(0x4A, 0x80) != to_write_val)
		is_3ds = true;

	// Restore initial register state
	i2cWriteRegister(0x4A, 0x80, read_val);
	return is_3ds;
}

int main(void) {

	// read User Settings from firmware
	readUserSettings();
	irqInit();

	// Start the RTC tracking IRQ
	//initClockIRQ();
	fifoInit();

	SetYtrigger(80);

	installSystemFIFO();
	
	irqSet(IRQ_VCOUNT, VcountHandler);
	irqSet(IRQ_VBLANK, VblankHandler);

	irqEnable( IRQ_VBLANK | IRQ_VCOUNT);

	uint16_t debugger_value = 0;
	uint8_t is_3ds = 0;
	if (isDSiMode()) {
		i2cWriteRegister(0x4A, 0x12, 0x00);		// Press power-button for auto-reset
		i2cWriteRegister(0x4A, 0x70, 0x01);		// Bootflag = Warmboot/SkipHealthSafety
		debugger_value = *((volatile uint16_t*)0x04004024);
		is_3ds = is_device_3ds() ? 1 : 0;
	}
	fifoSendValue32(FIFO_USER_01, debugger_value | (is_3ds << (sizeof(debugger_value) * 8)));
	
	while (1) {
		swiWaitForVBlank();
	}
}

