#ifndef LAUNCH_ENGINE_OLD_STRUCTS_H
#define LAUNCH_ENGINE_OLD_STRUCTS_H

#include "launch_engine.h"

// Compat structs...

#define OLD_LAUNCH_ENGINE_DATA_V0 0x0000
#define OLD_LAUNCH_ENGINE_DATA_V1 0x0100
#define OLD_LAUNCH_ENGINE_DATA_V1_1 0x0101

struct launch_engine_data_t_1_0 {
	int scfgUnlock;
	int sdaccess;
	int twlmode;
	int twlclk;
	int twlvram;
	int twltouch;
	int soundFreq;
	int runCardEngine;
	int language;
} PACKED ALIGNED(4);

struct launch_engine_data_t_1_1 {
	int scfgUnlock;
	int sdaccess;
	int twlmode;
	int twlclk;
	int twlvram;
	int twltouch;
	int soundFreq;
	int runCardEngine;
	int language;
	int sleepMode;
} PACKED ALIGNED(4);

#endif // LAUNCH_ENGINE_OLD_STRUCTS_H
