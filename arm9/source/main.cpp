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
#include <string>

#include "nds_card.h"
#include "launch_engine.h"
#include "old_launch_engine_structs.h"

#ifndef GAMETITLE
#define GAMETITLE "SLOT1LAUNCH"
#endif
#ifndef GAMEGROUPID
#define GAMEGROUPID "00"
#endif
#ifndef GAMECODE
#define GAMECODE "SL1L"
#endif

#define SAVED_VERSION 0x01010000

#define DEFAULT_SCFGUNLOCK_DSI 1
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

#define DEFAULT_SLEEPMODE 1

#define DEFAULT_AUTOBOOT 1

#define DEFAULT_VALUE_GENERIC -1

#define FILENAME_APP_DATA "sd:/_nds/s1l1_data.bin"
#define FILENAME_NAND_APP_DATA_APPEND "data/private_hb.sav"

struct all_saved_data_t {
	uint32_t version;
	struct launch_engine_data_t launch_engine_data;
	int autoboot;
} PACKED ALIGNED(4);

struct all_saved_data_t_1_0 {
	uint32_t version;
	struct launch_engine_data_t_1_0 launch_engine_data;
	int autoboot;
} PACKED ALIGNED(4);

struct all_options_data_t {
	struct all_saved_data_t all_saved_data;
	int reset_to_ds_mode;
	int reset_to_dsi_mode;
	int reset_to_defaults;
	int save_to_filepath;
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
	&all_options_data.all_saved_data.launch_engine_data.sleepMode,
	&all_options_data.all_saved_data.autoboot,
	&all_options_data.save_to_filepath,
	&all_options_data.reset_to_dsi_mode,
	&all_options_data.reset_to_ds_mode,
	&all_options_data.reset_to_defaults,
};

int *settings_options_ds[] = {
	&all_options_data.all_saved_data.launch_engine_data.sleepMode,
	&all_options_data.all_saved_data.launch_engine_data.language,
	&all_options_data.reset_to_defaults,
};

#define NUM_SETTINGS_OPTIONS_DSI (sizeof(settings_options_dsi) / sizeof(settings_options_dsi[0]))
#define NUM_SETTINGS_OPTIONS_DS (sizeof(settings_options_ds) / sizeof(settings_options_ds[0]))
#define SETTING_OPTIONS_VALUE_X_POSITION 19
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
	launch_engine_data->sleepMode = DEFAULT_SLEEPMODE;
}

void reset_all_saved_data(struct all_saved_data_t* all_saved_data) {
	all_saved_data->version = SAVED_VERSION;
	all_saved_data->launch_engine_data.twlmode = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twlclk = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twlvram = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.twltouch = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.language = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.soundFreq = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.sdaccess = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.scfgUnlock = DEFAULT_VALUE_GENERIC;
	all_saved_data->launch_engine_data.sleepMode = DEFAULT_SLEEPMODE;
	all_saved_data->autoboot = DEFAULT_AUTOBOOT;
}

void reset_all_options_data(struct all_options_data_t* all_options_data) {
	reset_all_saved_data(&all_options_data->all_saved_data);
	all_options_data->save_to_filepath = 0;
	all_options_data->reset_to_ds_mode = 0;
	all_options_data->reset_to_dsi_mode = 0;
	all_options_data->reset_to_defaults = 0;
	all_options_data->cursor_index = 0;
}

static void convert_saved_data_t0_t1_to_t11(struct all_saved_data_t* all_saved_data) {
	all_saved_data_t_1_0 old_saved_data = *((all_saved_data_t_1_0*)all_saved_data);
	all_saved_data->launch_engine_data.scfgUnlock = old_saved_data.launch_engine_data.scfgUnlock;
	all_saved_data->launch_engine_data.sdaccess = old_saved_data.launch_engine_data.sdaccess;
	all_saved_data->launch_engine_data.twlmode = old_saved_data.launch_engine_data.twlmode;
	all_saved_data->launch_engine_data.twlclk = old_saved_data.launch_engine_data.twlclk;
	all_saved_data->launch_engine_data.twlvram = old_saved_data.launch_engine_data.twlvram;
	all_saved_data->launch_engine_data.twltouch = old_saved_data.launch_engine_data.twltouch;
	all_saved_data->launch_engine_data.soundFreq = old_saved_data.launch_engine_data.soundFreq;
	all_saved_data->launch_engine_data.runCardEngine = old_saved_data.launch_engine_data.runCardEngine;
	all_saved_data->launch_engine_data.language = old_saved_data.launch_engine_data.language;
	all_saved_data->launch_engine_data.sleepMode = DEFAULT_SLEEPMODE;
	all_saved_data->autoboot = old_saved_data.autoboot;
	all_saved_data->version = SAVED_VERSION;
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
		while(REG_ROMCTRL & CARD_BUSY)
			swiWaitForVBlank();
		for(int i = 0; i < 2; i++)
			swiWaitForVBlank();
		REG_ROMCTRL = CARD_nRESET;
		for(int i = 0; i < 15; i++)
			swiWaitForVBlank();
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
	cardReadHeader((uint8*)&ndsHeader);
	if(is_read_header_right())
		return;
	cart_reset();
	cardReadHeader((uint8*)&ndsHeader);
}

bool is_card_ready(bool do_read) {
	if(do_read)
		cartridge_read_header_data_total();
	bool is_ready = is_read_header_right();
	if(!is_ready)
		return is_ready;
	// Wait for card to stablize before continuing
	for (int i = 0; i < 10; i++) { swiWaitForVBlank(); }
	return is_ready;
}

static void update_console_x_pos() {
	int x = 0;
	int y = 0;
	consoleGetCursor(NULL, &x, &y);
	consoleSetCursor(NULL, SETTING_OPTIONS_VALUE_X_POSITION, y);
}

void print_setting_option_three(int value, const char* option_name, const char* off_value, const char* on_value, const char* force_value) {
	PRINT_FUNCTION(option_name);
	PRINT_FUNCTION(":");
	update_console_x_pos();
	if(value == DEFAULT_VALUE_GENERIC)
		PRINT_FUNCTION("Default");
	else if(value == 0)
		PRINT_FUNCTION(off_value);
	else if(value == 1)
		PRINT_FUNCTION(on_value);
	else
		PRINT_FUNCTION(force_value);
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

void print_data(uint16_t debugger_type, struct all_options_data_t* all_options_data, bool can_save) {
	swiWaitForVBlank();
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
	PRINT_FUNCTION("Settings:");
	for(size_t i = 0; i < size_settings; i++) {
		PRINT_FUNCTION("\n ");
		if(((size_t)all_options_data->cursor_index) == i)
			PRINT_FUNCTION("<");
		else
			PRINT_FUNCTION(" ");
		if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlmode))
			print_setting_option_three(*settings_options[i], "DS/DSi Mode", "DS", "DSi", "Force DSi");
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
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.sleepMode))
			print_setting_option(*settings_options[i], "Sleep", "Off", "On");
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.language))
			print_language_to_console(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.autoboot))
			print_setting_option(*settings_options[i], "Autoboot", "Off", "On");
		else if(settings_options[i] == (&all_options_data->save_to_filepath)) {
			if(can_save)
				print_setting_option(*settings_options[i], "Save default to", "SD", "NAND");
			else
				PRINT_FUNCTION("Impossible to save default");
		}
		else if(settings_options[i] == (&all_options_data->reset_to_ds_mode))
			PRINT_FUNCTION("Set DS Mode Defaults");
		else if(settings_options[i] == (&all_options_data->reset_to_dsi_mode))
			PRINT_FUNCTION("Set DSi Mode Defaults");
		else if(settings_options[i] == (&all_options_data->reset_to_defaults))
			PRINT_FUNCTION("Set Default Settings");
		if(((size_t)all_options_data->cursor_index) == i)
			PRINT_FUNCTION(">");
	}
}

static bool write_data_to_path(const char* filepath, struct all_options_data_t* all_options_data) {
	FILE* file_write = fopen_mkdir(filepath, "wb");

	if(!file_write)
		return false;

	all_options_data->all_saved_data.version = SAVED_VERSION;
	fwrite(&all_options_data->all_saved_data, 1, sizeof(struct all_saved_data_t), file_write);
	fclose(file_write);
	return true;
}

static bool read_data_from_path(const char* filepath, struct all_options_data_t* all_options_data) {
	FILE* file_read = fopen(filepath, "rb");

	if(!file_read)
		return false;

	size_t read_data = fread(&all_options_data->all_saved_data, 1, sizeof(struct all_saved_data_t), file_read);

	fclose(file_read);

	if(read_data != sizeof(struct all_saved_data_t)) {
		// Try reading v1 data regardless...
		if(read_data != sizeof(struct all_saved_data_t_1_0)) {
			reset_all_options_data(all_options_data);
			return false;
		}
	}

	uint8_t main_and_sub_version = all_options_data->all_saved_data.version >> 16;

	if(main_and_sub_version != (SAVED_VERSION >> 16)) {
		// For now, accept v0 and v1... The data is properly formatted at the start
		// of the file...
		if((main_and_sub_version == OLD_LAUNCH_ENGINE_DATA_V0) || (main_and_sub_version == OLD_LAUNCH_ENGINE_DATA_V1))
			convert_saved_data_t0_t1_to_t11(&all_options_data->all_saved_data);
		else {
			reset_all_options_data(all_options_data);
			return false;
		}
	}

	return true;
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

static void input_processing(u32 curr_keys, struct all_options_data_t* all_options_data, bool has_sd_access, bool has_nand_access, std::string &base_title_nand_path) {
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
			fix_data_x_val_default(settings_options[i], 3);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlclk))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twlvram))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.twltouch))
			fix_data_two_val_default(settings_options[i]);
		else if(settings_options[i] == (&all_options_data->all_saved_data.launch_engine_data.sleepMode))
			fix_data_bool_val(settings_options[i]);
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
	if((curr_keys & KEY_LEFT) || (curr_keys & KEY_RIGHT)) {
		int i = all_options_data->cursor_index;
		if(settings_options[i] == (&all_options_data->save_to_filepath)) {
			if(!has_nand_access)
				*settings_options[i] = 0;
			else if(!has_sd_access)
				*settings_options[i] = 1;
			else
				fix_data_bool_val(settings_options[i]);
		}
	}
	if(curr_keys & KEY_A) {
		int i = all_options_data->cursor_index;
		if(settings_options[i] == (&all_options_data->save_to_filepath)) {
			// Undo generic "to the right" from general code above
			*settings_options[i] -= 1;
			if(has_sd_access && ((*settings_options[i]) == 0)) {
				swiWaitForVBlank();
				PRINT_FUNCTION("Saving to SD...");
				write_data_to_path(FILENAME_APP_DATA, all_options_data);
			}
			if(has_nand_access && ((*settings_options[i]) == 1) && (base_title_nand_path != "")) {
				swiWaitForVBlank();
				PRINT_FUNCTION("Saving to NAND...");
				nand_WriteProtect(false);
				write_data_to_path((base_title_nand_path + FILENAME_NAND_APP_DATA_APPEND).c_str(), all_options_data);
				nand_WriteProtect(true);
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

static bool are_same_strings_with_trailing_0s(const char* base, const char* cmp, size_t max_size) {
	size_t base_len = strnlen(base, max_size);
	if(base_len != strnlen(cmp, max_size))
		return false;
	if(memcmp(base, cmp, base_len) != 0)
		return false;
	for(size_t i = base_len; i < max_size; i++)
		if(base[i] != '\0')
			return false;
	return true;
}

static bool has_been_booted_from_nand(const char* app_filepath) {
	size_t app_filepath_len = strlen(app_filepath);
	const char nand_base_path[] = "nand:/title/";
	size_t nand_base_path_len = strlen(nand_base_path);
	if(app_filepath_len < nand_base_path_len)
		return false;
	if(memcmp(app_filepath, nand_base_path, nand_base_path_len) != 0)
		return false;
	const char app_extension[] = ".app";
	size_t app_extension_len = strlen(app_extension);
	return memcmp(app_filepath + (app_filepath_len - app_extension_len), app_extension, app_extension_len) == 0;
}

static std::string get_base_title_nand_path(const char* app_filepath) {
	size_t app_filepath_len = strlen(app_filepath);
	const char content_base_path[] = "/content/";
	size_t content_path_len = strlen(content_base_path);

	int second_last_pos_sep = -2;
	int last_pos_sep = -1;
	for(size_t i = 0; i < app_filepath_len; i++) {
		if(app_filepath[i] == '/') {
			second_last_pos_sep = last_pos_sep;
			last_pos_sep = (size_t)i;
		}
	}

	if(second_last_pos_sep <= 0)
		return "";

	if(memcmp(app_filepath + second_last_pos_sep, content_base_path, content_path_len) != 0)
		return "";

	return std::string(app_filepath, second_last_pos_sep + 1);
}

static void clean_cheat_data(uintptr_t cheat_data_address) {
	memset((void*)cheat_data_address, 0, 0x8000);
	*((u32*)cheat_data_address) = 0xCF000000;
}

int main(int argc, char **argv) {
	fifoSendValue32(FIFO_PM, PM_REQ_SLEEP_DISABLE);

	reset_all_options_data(&all_options_data);
	uint16_t debugger_type = 0;

	// Reset cheat data, since we won't be using it regardless...
	clean_cheat_data(0x023F0000);
	u32 curr_keys = 0;
	do {
		swiWaitForVBlank();
		scanKeys();
		curr_keys = keysHeld();
	} while((curr_keys & (KEY_DEBUG | KEY_SELECT)) && (!(curr_keys & KEY_B)));

	bool done = false;
	bool booted_from_nand = false;
	std::string base_title_nand_path = "";
	if(argc > 0) {
		booted_from_nand = has_been_booted_from_nand(argv[0]);
		if(booted_from_nand)
			base_title_nand_path = get_base_title_nand_path(argv[0]);
		if(base_title_nand_path == "")
			booted_from_nand = false;
	}

	volatile bool has_sd_access = false;
	volatile bool has_nand_access = false;
	if(isDSiMode()) {
		has_sd_access = fatInitDefault() && (access("sd:/", F_OK) == 0);
		has_nand_access = booted_from_nand && nandInit(true) && (access("nand:/", F_OK) == 0);
	}

	bool successful_read = false;
	bool read_from_nand = false;

	// Prioritize SD, so it's easier for users...
	if((!successful_read) && has_sd_access)
		successful_read = read_data_from_path(FILENAME_APP_DATA, &all_options_data);

	if((!successful_read) && has_nand_access) {
		successful_read = read_data_from_path((base_title_nand_path + FILENAME_NAND_APP_DATA_APPEND).c_str(), &all_options_data);
		read_from_nand = successful_read;
	}

	if(read_from_nand || (has_nand_access && (!has_sd_access)))
		all_options_data.save_to_filepath = 1;

	if(all_options_data.all_saved_data.autoboot == DEFAULT_VALUE_GENERIC)
		all_options_data.all_saved_data.autoboot = DEFAULT_AUTOBOOT;

	sysSetCardOwner (BUS_OWNER_ARM9);
	cartridge_read_header_data_total();

	if((!(curr_keys & KEY_B)) && all_options_data.all_saved_data.autoboot)
		done = is_card_ready(false);

	if(done) {
		const auto& gameCode = ndsHeader.header.gameCode;
		const auto& gameTitle = ndsHeader.header.gameTitle;
		const auto& makerCode = ndsHeader.header.makercode;
		if(are_same_strings_with_trailing_0s(gameCode, GAMECODE, 4) && are_same_strings_with_trailing_0s(makerCode, GAMEGROUPID, 2) && are_same_strings_with_trailing_0s(gameTitle, GAMETITLE, 12))
			done = false;
	} 

	if(!done) {
		videoSetMode(MODE_0_2D);
		consoleDemoInit();
		while(!fifoCheckValue32(FIFO_USER_01));
		debugger_type = fifoGetValue32(FIFO_USER_01);
		print_data(debugger_type, &all_options_data, has_sd_access || has_nand_access);
	}
	while(!done) {
		do {
			swiWaitForVBlank();
			scanKeys();
			curr_keys = keysDown();
		} while(!(curr_keys & ( KEY_X | KEY_Y | KEY_R | KEY_L | KEY_LEFT | KEY_RIGHT | KEY_UP | KEY_DOWN | KEY_A | KEY_START)));

		input_processing(curr_keys, &all_options_data, has_sd_access, has_nand_access, base_title_nand_path);
		print_data(debugger_type, &all_options_data, has_sd_access || has_nand_access);

		if(curr_keys & KEY_START) {
			done = is_card_ready(true);
		}
	}

	const auto& gameCode = ndsHeader.header.gameCode;
	bool useAltBootloader = memcmp(gameCode, "AMFE", 4) == 0 || memcmp(gameCode, "ALXX", 4) == 0;
	setup_defaults(&all_options_data.all_saved_data.launch_engine_data);
	runLaunchEngine(&all_options_data.all_saved_data.launch_engine_data, useAltBootloader);

	return 0;
}

