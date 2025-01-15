#include "debug_payloads.h"
#include <stdint.h>
#include <stdbool.h>

/*
static uint32_t curr_payload_pos = 0x02200000;

static void write_le32(volatile uint8_t* ptr, uint32_t value) {
	for(int i = 0; i < 4; i++)
		ptr[i] = (value >> (8 * i)) & 0xFF;
}

static void write_le16(volatile uint8_t* ptr, uint16_t value) {
	for(int i = 0; i < 2; i++)
		ptr[i] = (value >> (8 * i)) & 0xFF;
}

static void create_connection_to_pos(volatile uint8_t* write_pos, uintptr_t jump_pos, uintptr_t payload_pos, bool is_thumb) {
	uint32_t divider = 2;
	if(!is_thumb)
		divider = 4;
	uint32_t subtracter = divider * 2;
	uintptr_t target_diff = ((payload_pos - (((jump_pos + 3) / 4) * 4)) - subtracter) / divider;
	if(!is_thumb)
		write_le32(write_pos, 0xEB000000 | (target_diff & 0x00FFFFFF));
	else {
		uint16_t code_bottom = 0xF000;
		uint16_t code_top = 0xE800;
		code_top |= target_diff & 0x7FF;
		code_bottom |= (target_diff >> 11) & 0xFFF;
		write_le16(write_pos, code_bottom);
		write_le16(write_pos + 2, code_top);
	}
}
*/

void insert_arm9_payload() {
	/*
	//ARM9
	#define NUM_CONNECT_INST 1
	#define NUM_INST (41 + NUM_CONNECT_INST)

	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE3A00301, 0xE3A00301, 0xE3A00301, 0xE2800D09, 0xE3A01081, 0xE5C01000, 0xE3A01084, 0xE5C01002, 0xE3A01C82, 0xE2811003, 0xE58010C4, 0xE3A00301, 0xE3A01801, 0xE2811EF1, 0xE5801000, 0xE2800A01, 0xE5801000, 0xE280006C, 0xE3A01000, 0xE5801000, 0xE2400A01, 0xE5801000, 0xE3A01405, 0xE3A0001F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE3A01301, 0xE2811E13, 0xE5910000, 0xE2000001, 0xE3500001, 0x0AFFFFFB, 0xE3A01405, 0xE3A00B1F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE3A00000,
	// Returns back
	0xE12FFF1E};
	*/
	/*
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE3A00301, 0xE3A00301, 0xE3A00301, 0xE3A01801, 0xE2811EF1, 0xE5801000, 0xE2800A01, 0xE5801000, 0xE280006C, 0xE3A01000, 0xE5801000, 0xE2400A01, 0xE5801000, 0xE3A01405, 0xE3A0001F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE3A01301, 0xE2811E13, 0xE5910000, 0xE2000001, 0xE3500001, 0x0AFFFFFB, 0xE3A01405, 0xE3A00B1F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE5840038,
	// Returns back
	0xE12FFF1E};
	*/
	/*
	#define NUM_CONNECT_INST 1
	#define NUM_INST (32 + NUM_CONNECT_INST)
	#define POS_PAYLOAD_LOAD_ADDR 7
	#define POS_PAYLOAD_CMP_ADDR (POS_PAYLOAD_LOAD_ADDR + 1)
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE59F0010, 0xE59F1010, 0xE5900000, 0xE1500001, 0x0A000002, 0xEAFFFFFE, 0, 0, 0xE3A00301, 0xE3A01801, 0xE2811EF1, 0xE5801000, 0xE2800A01, 0xE5801000, 0xE280006C, 0xE3A01000, 0xE5801000, 0xE2400A01, 0xE5801000, 0xE3A01405, 0xE3A0001F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE3A01301, 0xE2811E13, 0xE5910000, 0xE2000001, 0xE3500001, 0x0AFFFFFB, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE1A00007,
	// Returns back
	0xE12FFF1E};
	*/
	/*
	#define NUM_CONNECT_COPY_INST 1
	#define NUM_COPY_INST (8 + NUM_CONNECT_COPY_INST)
	uint32_t instructions_copy_payload[NUM_COPY_INST] = {0, 0, 0xE92D0003, 0xE51F0010, 0xE51F1018, 0xE5801000, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE5810000,
	// Returns back
	0xE12FFF1E};
	volatile uint8_t* payload_pos= (volatile uint8_t*)curr_payload_pos;
	curr_payload_pos += (NUM_INST * 4);
	volatile uint8_t* payload_copy_pos = (volatile uint8_t*)curr_payload_pos;
	curr_payload_pos += (NUM_COPY_INST * 4);
	for(int i = 0; i < NUM_INST; i++)
		write_le32(payload_pos + (i * 4), instructions[i]);
	for(int i = 0; i < NUM_COPY_INST; i++)
		write_le32(payload_copy_pos + (i * 4), instructions_copy_payload[i]);
	volatile uint8_t* copy_jump_pos = (volatile uint8_t*)0x020008F0;
	volatile uint8_t* jump_pos = (volatile uint8_t*)0x020D7C2C;
	//volatile uint8_t* jump_pos = (volatile uint8_t*)0x0225E680;
	volatile uint8_t* cmp_target_pos = (volatile uint8_t*)0x02215BE0;
	volatile uint32_t cmp_expected = (volatile uint8_t*)0xE59F000D;
	//create_connection_to_pos(jump_pos, jump_pos, payload_pos, false);
	create_connection_to_pos(copy_jump_pos, copy_jump_pos, payload_copy_pos + 8, false);
	create_connection_to_pos(payload_copy_pos, jump_pos, payload_pos, false);
	write_le32(payload_copy_pos + 4, jump_pos);
	//write_le32(payload_pos + (4 * POS_PAYLOAD_LOAD_ADDR), cmp_target_pos);
	//write_le32(payload_pos + (4 * POS_PAYLOAD_CMP_ADDR), cmp_expected);
	*/

	/*
	//ARM9
	#define NUM_CONNECT_INST 1
	#define NUM_INST (20 + NUM_CONNECT_INST)
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE3A00301, 0xE3A01801, 0xE2811EF1, 0xE5801000, 0xE2800A01, 0xE5801000, 0xE3A01405, 0xE3A0001F, 0xE1C100B0, 0xE2811B01, 0xE1C100B0, 0xE3A01301, 0xE2811E13, 0xE5910000, 0xE2000001, 0xE3500001, 0x0AFFFFFB, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE3A01003,
	// Returns back
	0xE12FFF1E};
	#define NUM_CONNECT_COPY_INST 1
	#define NUM_COPY_INST (8 + NUM_CONNECT_COPY_INST)
	uint32_t instructions_copy_payload[NUM_COPY_INST] = {0, 0, 0xE92D0003, 0xE51F0010, 0xE51F1018, 0xE5801000, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE3A01000,
	// Returns back
	0xE12FFF1E};
	volatile uint8_t* payload_pos= (volatile uint8_t*)0x02200000;
	volatile uint8_t* payload_copy_pos = payload_pos + (NUM_INST * 4);
	for(int i = 0; i < NUM_INST; i++)
		write_le32(payload_pos + (i * 4), instructions[i]);
	for(int i = 0; i < NUM_COPY_INST; i++)
		write_le32(payload_copy_pos + (i * 4), instructions_copy_payload[i]);
	volatile uint8_t* copy_jump_pos = (volatile uint8_t*)0x02004954;
	volatile uint8_t* jump_pos = (volatile uint8_t*)0x0200F22E;
	//create_connection_to_pos(jump_pos, jump_pos, payload_pos, false);
	create_connection_to_pos(copy_jump_pos, copy_jump_pos, payload_copy_pos + 8, false);
	create_connection_to_pos(payload_copy_pos, jump_pos, payload_pos, true);
	write_le32(payload_copy_pos + 4, jump_pos);
	*/
}

void insert_arm7_payload() {
	//ARM7
	/*
	#define NUM_CONNECT_INST 0
	#define NUM_INST (10 + NUM_CONNECT_INST)
	#define POS_PAYLOAD_CMP_ADDR 7
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE5976000, 0xE59F000C, 0xE1500006, 0x1A000002, 0xEF070000, 0xEAFFFFFD, 0, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	
	// Returns back
	0xE12FFF1E
	};
	*/
	/*
	#define NUM_CONNECT_INST 0
	#define NUM_INST (9 + NUM_CONNECT_INST)
	//#define POS_PAYLOAD_CMP_ADDR 7
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE5976000, 0xE206001F, 0xE3500005, 0x1A000001, 0xEF070000, 0xEAFFFFFD, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	
	// Returns back
	0xE12FFF1E
	};
	*/
	/*
	#define NUM_CONNECT_INST 1
	#define NUM_INST (12 + NUM_CONNECT_INST)
	//#define POS_PAYLOAD_CMP_ADDR 7
	#define POS_PAYLOAD_NUM_ADDR 7
	uint32_t instructions[NUM_INST] = {0xE92D0003, 0xE59F1010, 0xE5910000, 0xE2500001, 0x1A000003, 0xEF070000, 0xEAFFFFFD, 0, 1, 0xE5810000, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE5976000,
	// Returns back
	0xE12FFF1E
	};
	*/
	/*
	#define NUM_CONNECT_INST 0
	#define NUM_INST (2 + NUM_CONNECT_INST)
	uint32_t instructions[NUM_INST] = {0xEF070000, 0xEAFFFFFD
	// End of payload, place instructions to connect back below...
	// Returns back
	};
	*/
	/*
	#define NUM_CONNECT_COPY_COPY_INST 1
	#define NUM_COPY_COPY_INST (8 + NUM_CONNECT_COPY_COPY_INST)
	uint32_t instructions_copy_copy_payload[NUM_COPY_COPY_INST] = {0xEAFFFFFE, 0, 0xE92D0003, 0xE51F0010, 0xE51F1018, 0xE5801000, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE5810000,
	// Returns back
	0xE12FFF1E};

	#define NUM_CONNECT_COPY_INST 1
	#define NUM_COPY_INST (8 + NUM_CONNECT_COPY_INST)
	uint32_t instructions_copy_payload[NUM_COPY_INST] = {0xEAFFFFFE, 0, 0xE92D0003, 0xE51F0010, 0xE51F1018, 0xE5801000, 0xE8BD0003,
	// End of payload, place instructions to connect back below...
	0xE3A00000,
	// Returns back
	0xE12FFF1E};
	volatile uint8_t* payload_pos= (volatile uint8_t*)curr_payload_pos;
	curr_payload_pos += (NUM_INST * 4);
	volatile uint8_t* payload_copy_pos = (volatile uint8_t*)curr_payload_pos;
	curr_payload_pos += (NUM_COPY_INST * 4);
	volatile uint8_t* payload_copy_copy_pos = (volatile uint8_t*)curr_payload_pos;
	curr_payload_pos += (NUM_COPY_COPY_INST * 4);
	for(int i = 0; i < NUM_INST; i++)
		write_le32(payload_pos + (i * 4), instructions[i]);
	for(int i = 0; i < NUM_COPY_INST; i++)
		write_le32(payload_copy_pos + (i * 4), instructions_copy_payload[i]);
	for(int i = 0; i < NUM_COPY_COPY_INST; i++)
		write_le32(payload_copy_copy_pos + (i * 4), instructions_copy_copy_payload[i]);
	volatile uint8_t* copy_copy_jump_pos = (volatile uint8_t*)0x02380104;
	volatile uint8_t* copy_jump_pos = (volatile uint8_t*)0x037F8010;
	volatile uint8_t* jump_pos = (volatile uint8_t*)0x03807B04;
	//volatile uint32_t cmp_expected = (volatile uint8_t*)0x82EA4807;
	volatile uint32_t cmp_expected = (volatile uint8_t*)0x82EA4807;
	//create_connection_to_pos(jump_pos, jump_pos, payload_pos, false);
	//create_connection_to_pos(copy_copy_jump_pos, copy_copy_jump_pos, payload_copy_copy_pos + 8, false);
	//create_connection_to_pos(payload_copy_copy_pos, copy_jump_pos, payload_copy_pos + 8, false);
	//write_le32(payload_copy_copy_pos + 4, copy_jump_pos);
	create_connection_to_pos(copy_copy_jump_pos, copy_copy_jump_pos, payload_copy_copy_pos + 8, false);
	//create_connection_to_pos(payload_copy_pos, jump_pos, payload_pos, false);
	write_le32(payload_copy_copy_pos + 4, jump_pos);
	//write_le32(payload_pos + (4 * POS_PAYLOAD_CMP_ADDR), cmp_expected);
	//write_le32(payload_pos + (4 * POS_PAYLOAD_NUM_ADDR), payload_pos + (4 * (POS_PAYLOAD_NUM_ADDR + 1)));
	*/
}
