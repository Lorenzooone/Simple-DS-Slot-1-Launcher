#ifndef __BOOTLOADER_SHARED_DEFS
#define __BOOTLOADER_SHARED_DEFS

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif
#ifndef ALIGNED
#define ALIGNED(x) __attribute__((aligned(x)))
#endif

//#define DO_BOOTLOADER_DEBUG_PRINTS

struct bootloader_main_data_t {
	const uint32_t first_jump;
	const uint32_t copy_location;
	const uint32_t copy_end;
	uint32_t dsiMode;
	uint32_t language;
	uint32_t sdAccess;
	uint32_t scfgUnlock;
	uint32_t twlMode;
	uint32_t twlClock;
	uint32_t boostVram;
	uint32_t twlTouch;
	uint32_t soundFreq;
	uint32_t runCardEngine;
	uint32_t sleepMode;
	uint32_t redirectPowerButton;
	uint32_t hasDoubleRAM;
	uint32_t cardEngineLocation;
	uint32_t cardEngineSize;
	uint8_t selfTitleId[8];
} PACKED ALIGNED(4);

#endif
