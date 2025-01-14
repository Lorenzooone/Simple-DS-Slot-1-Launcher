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
#include <fat.h>
#include <sys/stat.h>

#include <stdio.h>
#include <string.h>

#include "nds_card.h"
#include "launch_engine.h"

#define SAVED_VERSION 0x1000000

#define DEFAULT_SCFGUNLOCK_DSI 0
#define DEFAULT_SDACCESS_DSI 1
#define DEFAULT_TWLMODE_DSI 1
#define DEFAULT_TWLCLK_DSI 1
#define DEFAULT_TWLVRAM_DSI 1
#define DEFAULT_TWLTOUCH_DSI 1
#define DEFAULT_SOUNDFREQ_DSI 0
#define DEFAULT_CARDENGINE_DSI 0

#define DEFAULT_SCFGUNLOCK_DS 0
#define DEFAULT_SDACCESS_DS 0
#define DEFAULT_TWLMODE_DS 0
#define DEFAULT_TWLCLK_DS 0
#define DEFAULT_TWLVRAM_DS 0
#define DEFAULT_TWLTOUCH_DS 0
#define DEFAULT_SOUNDFREQ_DS 0
#define DEFAULT_CARDENGINE_DS 0

#define DEFAULT_AUTOBOOT 1

#define DEFAULT_VALUE_GENERIC -1

#define FILENAME_APP_DATA "sd:/_nds/s1l1_data.bin"

struct all_saved_data_t {
	uint32_t version;
	struct launch_engine_data_t launch_engine_data;
	int autoboot;
} PACKED ALIGNED(4);

struct all_options_data_t {
	struct all_saved_data_t all_saved_data;
	int reset_to_ds_mode;
	int reset_to_dsi_mode;
	int reset_to_defaults;
	int save_to_sd;
	int cursor_index;
};

struct {
sNDSHeader header;
char padding[0x200 - sizeof(sNDSHeader)];
} ndsHeader;

struct all_options_data_t all_options_data;

int *settings_options_dsi[] = {
	&all_options_data.all_saved_data.launch_engine_data.twlmode,
	&all_options_data.all_saved_data.launch_engine_data.twlclk,
	&all_options_data.all_saved_data.launch_engine_data.twlvram,
	&all_options_data.all_saved_data.launch_engine_data.twltouch,
	&all_options_data.all_saved_data.launch_engine_data.language,
	&all_options_data.all_saved_data.launch_engine_data.soundFreq,
	&all_options_data.all_saved_data.launch_engine_data.sdaccess,
	&all_options_data.all_saved_data.launch_engine_data.scfgUnlock,
	&all_options_data.all_saved_data.autoboot,
	&all_options_data.save_to_sd,
	&all_options_data.reset_to_dsi_mode,
	&all_options_data.reset_to_ds_mode,
	&all_options_data.reset_to_defaults,
};

int *settings_options_ds[] = {
	&all_options_data.all_saved_data.launch_engine_data.language,
	&all_options_data.reset_to_defaults,
};

#define NUM_SETTINGS_OPTIONS_DSI (sizeof(settings_options_dsi) / sizeof(settings_options_dsi[0]))
#define NUM_SETTINGS_OPTIONS_DS (sizeof(settings_options_ds) / sizeof(settings_options_ds[0]))
#define SETTING_OPTIONS_VALUE_X_POSITION 16
#define PRINT_FUNCTION printf
#define NUM_LANGUAGES_DS 7
#define NUM_LANGUAGES_STRINGS (NUM_LANGUAGES_DS + 1)
const char* languages_strings[NUM_LANGUAGES_STRINGS] = {"Default", "Japanese", "English", "French", "German", "Italian", "Spanish", "Chinese"};

void reset_launch_engine_data_to_dsx(struct launch_engine_data_t* launch_engine_data, bool to_ds) {
	launch_engine_data->twlmode = to_ds ? DEFAULT_TWLMODE_DS : DEFAULT_TWLMODE_DSI;
	launch_engine_data->twlclk = to_ds ? DEFAULT_TWLCLK_DS : DEFAULT_TWLCLK_DSI;
	launch_engine_data->twlvram = to_ds ? DEFAULT_TWLVRAM_DS : DEFAULT_TWLVRAM_DSI;
	launch_engine_data->twltouch = to_ds ? DEFAULT_TWLTOUCH_DS : DEFAULT_TWLTOUCH_DSI;
	launch_engine_data->soundFreq = to_ds ? DEFAULT_SOUNDFREQ_DS : DEFAULT_SOUNDFREQ_DSI;
	launch_engine_data->sdaccess = to_ds ? DEFAULT_SDACCESS_DS : DEFAULT_SDACCESS_DSI;
	launch_engine_data->scfgUnlock = to_ds ? DEFAULT_SCFGUNLOCK_DS : DEFAULT_SCFGUNLOCK_DSI;
}

void reset_all_saved_data(struct all_saved_data_t* all_saved_data) {
	all_saved_data->launch_engine_data.twlmode = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twlclk = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twlvram = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twltouch = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.language = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.soundFreq = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.sdaccess = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.scfgUnlock = DEFAULT_VALUE_GENERIC;
	all_saved_data->autoboot = DEFAULT_AUTOBOOT;
}

void reset_all_options_data(struct all_options_data_t* all_options_data) {
	reset_all_saved_data(&all_options_data->all_saved_data);
	all_options_data->save_to_sd = 0;
	all_options_data->reset_to_ds_mode = 0;
	all_options_data->reset_to_dsi_mode = 0;
	all_options_data->reset_to_defaults = 0;
	all_options_data->cursor_index = 0;
}

void rek_mkdir(char *path) {
    char *sep = strrchr(path, '/');
    if(sep != NULL) {
        *sep = 0;
        rek_mkdir(path);
        *sep = '/';
    }
    mkdir(path, 0755);
}

FILE *fopen_mkdir(const char *path, const char *mode) {
    char *sep = strrchr(path, '/');
    if(sep) { 
        char *path0 = strdup(path);
        path0[ sep - path ] = 0;
        rek_mkdir(path0);
        free(path0);
    }
    return fopen(path,mode);
}

void cart_reset() {
	if(isDSiMode()) {
		disableSlot1();
		for(int i = 0; i < 10; i++)
			swiWaitForVBlank();
		enableSlot1();
	}
}

static bool is_dsi_cartridge() {
	return ndsHeader.header.unitCode > 0;
}

static void set_default_val(int* value, uint8_t default_ds, uint8_t default_dsi) {
	if((*value) == DEFAULT_VALUE_GENERIC)
		*value = is_dsi_cartridge() ? default_dsi : default_ds;
}

static void setup_defaults(struct launch_engine_data_t* launch_engine_data) {
	launch_engine_data->runCardEngine = 0;
	if(!isDSiMode()) {
		launch_engine_data->scfgUnlock = 0;
		launch_engine_data->sdaccess = 0;
		launch_engine_data->twlmode = 0;
		launch_engine_data->twlclk = 0;
		launch_engine_data->twlvram = 0;
		launch_engine_data->twltouch = 0;
		launch_engine_data->soundFreq = 0;
		return;
	}
	set_default_val(&launch_engine_data->scfgUnlock, DEFAULT_SCFGUNLOCK_DS, DEFAULT_SCFGUNLOCK_DSI);
	set_default_val(&launch_engine_data->sdaccess, DEFAULT_SDACCESS_DS, DEFAULT_SDACCESS_DSI);
	set_default_val(&launch_engine_data->twlmode, DEFAULT_TWLMODE_DS, DEFAULT_TWLMODE_DSI);
	set_default_val(&launch_engine_data->twlclk, DEFAULT_TWLCLK_DS, DEFAULT_TWLCLK_DSI);
	set_default_val(&launch_engine_data->twlvram, DEFAULT_TWLVRAM_DS, DEFAULT_TWLVRAM_DSI);
	set_default_val(&launch_engine_data->twltouch, DEFAULT_TWLTOUCH_DS, DEFAULT_TWLTOUCH_DSI);
	set_default_val(&launch_engine_data->soundFreq, DEFAULT_SOUNDFREQ_DS, DEFAULT_SOUNDFREQ_DSI);
}

static bool is_read_header_right() {
	return ndsHeader.header.headerCRC16 == swiCRC16(0xFFFF, (void*)&ndsHeader, 0x15E);
}

static void cartridge_read_header_data_total() {
	cart_reset();
	cardReadHeader((uint8*)&ndsHeader);
}

bool is_card_ready(bool do_read) {
	if(do_read)
		cartridge_read_header_data_total();
	// Wait for card to stablize before continuing
	for (int i = 0; i < 30; i++) { swiWaitForVBlank(); }
	// Check header CRC
	return is_read_header_right();
}

static void update_console_x_pos() {
	int x = 0;
	int y = 0;
	consoleGetCursor(NULL, &x, &y);
	consoleSetCursor(NULL, SETTING_OPTIONS_VALUE_X_POSITION, y);
}

void print_setting_option(int value, const char* option_name, const char* off_value, const char* on_value) {
	PRINT_FUNCTION(option_name);
	PRINT_FUNCTION(":");
	update_console_x_pos();
	if(value == DEFAULT_VALUE_GENERIC)
		PRINT_FUNCTION("Default");
	else if(value == 0)
		PRINT_FUNCTION(off_value);
	else
		PRINT_FUNCTION(on_value);
}

static void print_language_to_console(int* language_selected) {
	if(((*language_selected) < DEFAULT_VALUE_GENERIC) || ((*language_selected) >= NUM_LANGUAGES_DS))
		*language_selected = DEFAULT_VALUE_GENERIC;
	PRINT_FUNCTION("Language:");
	update_console_x_pos();
	PRINT_FUNCTION(languages_strings[(*language_selected) + 1]);
}

void print_data(uint16_t debugger_type, struct all_options_data_t* all_options_data) {
	consoleClear();
	int ram_size = 4;
	if(isDSiMode()) {
		PRINT_FUNCTION ("DSi - ");
		ram_size = 16;
	}
	else
		PRINT_FUNCTION ("DS - ");
	if(isHwDebugger()) {
		ram_size *= 2;
		if(!isDSiMode())
			debugger_type = 1;
	}

	PRINT_FUNCTION ("%dMB RAM", ram_size);
	if(debugger_type)
		PRINT_FUNCTION (" - Dev\n");
	else
		PRINT_FUNCTION (" - Retail\n");
	PRINT_FUNCTION ("Please insert a DS cartridge.\n\n");
	PRINT_FUNCTION ("X/Y/L/R: Update Information.\n");
	PRINT_FUNCTION ("START: Try to launch the game.\n\n");
	bool success = true;
	if(*(u32*)&ndsHeader == 0xffffffff)
		success = false;
	if(!is_read_header_right())
		success = false;
	char name[256];
	sprintf(&name[0], "----");
	if(success) {
		memcpy(&name[0], &ndsHeader.header.gameCode[0], 4);
		name[4] = 0x00;
	}
	PRINT_FUNCTION("ID: %s", name);
	if(success) {
		if(is_dsi_cartridge())
			PRINT_FUNCTION(" - DSi Cart");
		else
			PRINT_FUNCTION(" - DS Cart");
	}
	sprintf(&name[0], "------------");
	if(success) {
		memcpy(&name[0], &ndsHeader.header.gameTitle[0], 12);
		name[12] = 0x00;
	}
	PRINT_FUNCTION("\nName: %s\n\n", name);

	int **settings_options = settings_options_dsi;
	size_t size_settings = NUM_SETTINGS_OPTIONS_DSI;
	if(!isDSiMode()) {
		settings_options = settings_options_ds;
		size_settings = NUM_SETTINGS_OPTIONS_DS;
	}
	PRINT_FUNCTION("Settings:\n");
	for(size_t i = 0; i < size_settings; i++) {
		PRINT_FUNCTION(" ");
		if(((size_t)all_options_data->cursor_index) == i)
			PRINT_FUNCTION("<");
		else
			PRINT_FUNCTION(" ");
		if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlmode))
			print_setting_option(*settings_options[i], "DS/DSi Mode", "DS", "DSi");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlclk))
			print_setting_option(*settings_options[i], "CPU Speed", "67MHz", "133MHz");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlvram))
			print_setting_option(*settings_options[i], "VRAM Boost", "Off", "On");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twltouch))
			print_setting_option(*settings_options[i], "Touchscreen", "DS", "DSi");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.sdaccess))
			print_setting_option(*settings_options[i], "SD Access", "Off", "On");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.soundFreq))
			print_setting_option(*settings_options[i], "Sound Freq.", "32KHz", "48KHz");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.scfgUnlock))
			print_setting_option(*settings_options[i], "SCFG", "Locked", "Unlocked");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.language))
			print_language_to_console(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.autoboot))
			print_setting_option(*settings_options[i], "Autoboot", "Off", "On");
		else if(settings_options[i] == (&all_options_data->save_to_sd))
			PRINT_FUNCTION("Save default to SD");
		else if(settings_options[i] == (&all_options_data->reset_to_ds_mode))
			PRINT_FUNCTION("Set DS Mode Defaults");
		else if(settings_options[i] == (&all_options_data->reset_to_dsi_mode))
			PRINT_FUNCTION("Set DSi Mode Defaults");
		else if(settings_options[i] == (&all_options_data->reset_to_defaults))
			PRINT_FUNCTION("Set Default Settings");
		if(((size_t)all_options_data->cursor_index) == i)
			PRINT_FUNCTION(">");
		PRINT_FUNCTION("\n");
	}
}

static void fix_data_x_val_default(int* value, int num_values) {
	if((*value) < DEFAULT_VALUE_GENERIC)
		*value = num_values - 1;
	if((*value) >= num_values)
		*value = DEFAULT_VALUE_GENERIC;
}

static void fix_data_two_val_default(int* value) {
	fix_data_x_val_default(value, 2);
}

static void fix_data_bool_val(int* value) {
	if((*value) < 0)
		*value = 1;
	if((*value) > 1)
		*value = 0;
}

static void input_processing(u32 curr_keys, struct all_options_data_t* all_options_data, bool has_sd_access) {
	if((curr_keys & KEY_X) || (curr_keys & KEY_Y) || (curr_keys & KEY_L) || (curr_keys & KEY_R))
		cartridge_read_header_data_total();

	int **settings_options = settings_options_dsi;
	size_t size_settings = NUM_SETTINGS_OPTIONS_DSI;
	if(!isDSiMode()) {
		settings_options = settings_options_ds;
		size_settings = NUM_SETTINGS_OPTIONS_DS;
	}

	if(curr_keys & KEY_UP)
		all_options_data->cursor_index -= 1;
	if(curr_keys & KEY_DOWN)
		all_options_data->cursor_index += 1;
	if(all_options_data->cursor_index < 0)
		all_options_data->cursor_index = size_settings - 1;
	if(((size_t)all_options_data->cursor_index) >= size_settings)
		all_options_data->cursor_index = 0;

	if(curr_keys & KEY_LEFT)
		*settings_options[all_options_data->cursor_index] -= 1;
	if(curr_keys & KEY_RIGHT)
		*settings_options[all_options_data->cursor_index] += 1;
	if(curr_keys & KEY_A)
		*settings_options[all_options_data->cursor_index] += 1;
	if((curr_keys & KEY_LEFT) || (curr_keys & KEY_RIGHT) || (curr_keys & KEY_A)) {
		int i = all_options_data->cursor_index;
		if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlmode))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlclk))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlvram))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twltouch))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.sdaccess))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.soundFreq))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.scfgUnlock))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.language))
			fix_data_x_val_default(settings_options[i], NUM_LANGUAGES_DS);
		else if(settings_options[i] == (&all_options_data->all_saved_data.autoboot))
			fix_data_bool_val(settings_options[i]);
	}
	if(curr_keys & KEY_A) {
		int i = all_options_data->cursor_index;
		if(settings_options[i] == (&all_options_data->save_to_sd)) {
			PRINT_FUNCTION("Saving...");
			if(has_sd_access) {
				FILE* file_write = fopen_mkdir(FILENAME_APP_DATA, "wb");
				if(file_write) {
					fwrite(all_options_data, 1, sizeof(struct all_options_data_t), file_write);
					fclose(file_write);
				}
			}
		}
		if(settings_options[i] == (&all_options_data->reset_to_ds_mode))
			reset_launch_engine_data_to_dsx(&all_options_data->all_saved_data.launch_engine_data, true);
		else if(settings_options[i] == (&all_options_data->reset_to_dsi_mode))
			reset_launch_engine_data_to_dsx(&all_options_data->all_saved_data.launch_engine_data, false);
		else if(settings_options[i] == (&all_options_data->reset_to_defaults))
			reset_all_saved_data(&all_options_data->all_saved_data);
	}
}

int main() {
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);

	reset_all_options_data(&all_options_data);
	uint16_t debugger_type = 0;
	// Reset cheat data, since we won't be using it regardless...
	memset((void*)0x023F0000, 0, 0x8000);
	u32 curr_keys = 0;
	do {
		swiWaitForVBlank();
		scanKeys();
		curr_keys = keysCurrent();
	} while((curr_keys & (KEY_DEBUG | KEY_SELECT)) && (!(curr_keys & KEY_B)));
	bool done = false;
	bool has_sd_access = false;

	if(isDSiMode())
		has_sd_access = fatInitDefault();
	if(has_sd_access) {
		FILE* file_read = fopen(FILENAME_APP_DATA, "rb");
		if(file_read) {
			size_t read_data = fread(&all_options_data, 1, sizeof(struct all_options_data_t), file_read);
			if(read_data != sizeof(struct all_options_data_t))
				reset_all_options_data(&all_options_data);
			fclose(file_read);
		}
	}
	if(all_options_data.all_saved_data.autoboot == DEFAULT_VALUE_GENERIC)
		all_options_data.all_saved_data.autoboot = DEFAULT_AUTOBOOT;

	sysSetCardOwner (BUS_OWNER_ARM9);
	if((!(curr_keys & KEY_B)) && all_options_data.all_saved_data.autoboot)
		done = is_card_ready(true);
	else
		cartridge_read_header_data_total();
		

	if(!done) {
		videoSetMode(MODE_0_2D);
		consoleDemoInit();
		while(!fifoCheckValue32(FIFO_USER_01));
		debugger_type = fifoGetValue32(FIFO_USER_01);
		print_data(debugger_type, &all_options_data);
	}
	while(!done) {
		do {
			swiWaitForVBlank();
			scanKeys();
			curr_keys = keysDown();
		} while(!(curr_keys & ( KEY_X | KEY_Y | KEY_R | KEY_L | KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN | KEY_A | KEY_START)));

		input_processing(curr_keys, &all_options_data, has_sd_access);
		print_data(debugger_type, &all_options_data);

		if(curr_keys & KEY_START) {
			done = is_card_ready(false);
			if(!done)
				done = is_card_ready(true);
		}
	}

	while (1) {
		const auto& gameCode = ndsHeader.header.gameCode;
		setup_defaults(&all_options_data.all_saved_data.launch_engine_data);
		runLaunchEngine(&all_options_data.all_saved_data.launch_engine_data, (memcmp(gameCode, "UBRP", 4) == 0 || memcmp(gameCode, "AMFE", 4) == 0 || memcmp(gameCode, "ALXX", 4) == 0 || memcmp(ndsHeader.header.gameTitle, "D!S!XTREME", 10) == 0), (memcmp(gameCode, "UBRP", 4) == 0));
	}
	return 0;
}

