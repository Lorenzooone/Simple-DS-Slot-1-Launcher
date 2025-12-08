#ifndef __RESET_TO_LOADER_H
#define __RESET_TO_LOADER_H

#include <stdint.h>
#include "cardengine_defs.h"

uint8_t* getSDResetData();
void setRebootParams(struct cardengine_main_data_t* cardengine_main_data, uint8_t* boot_path, uint8_t* reset_data);

#endif
