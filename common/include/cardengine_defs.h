#ifndef __CARDENGINE_SHARED_DEFS
#define __CARDENGINE_SHARED_DEFS

#define CARDENGINE_BOOT_TYPE_NOT_SUPPORTED 0
#define CARDENGINE_BOOT_TYPE_DSIWARE 1
#define CARDENGINE_BOOT_TYPE_SD 2

#ifndef PACKED
#define PACKED __attribute__((packed))
#endif
#ifndef ALIGNED
#define ALIGNED(x) __attribute__((aligned(x)))
#endif

struct cardengine_main_data_t {
	const uint32_t copy_location;
	const uint32_t copy_end;
	const uint32_t patches_offset;
	uint32_t intr_vblank_orig_return;
	uint32_t language;
	uint32_t gameSoftReset;
	uint32_t cheat_data_offset;
	uint32_t boot_type;
	uint32_t read_power_button;
	const uint32_t reset_data_offset;
	const uint32_t reset_data_len;
	const uint32_t boot_path_offset;
	const uint32_t boot_path_max_len;
} PACKED ALIGNED(4);

#endif
