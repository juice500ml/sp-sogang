#ifndef __HEXCTRL_H__
#define __HEXCTRL_H__

#include <stdbool.h>
#include <stdint.h>

uint32_t get_location (uint32_t, bool);
void autodump (void *, uint32_t, uint32_t);
void hexfill (uint8_t *, uint32_t, uint32_t, uint32_t, uint8_t);

#endif
