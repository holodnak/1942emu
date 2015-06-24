#ifndef __machine_h__
#define __machine_h__

#include "../deadz80/deadz80.h"

#define CPU0	0x00
#define CPU1	0x01
#define GFX0	0x10
#define GFX1	0x11
#define GFX2	0x12
#define PROM0	0x20

typedef unsigned __int64 u64;
typedef signed __int64 s64;

void decode_palette();
u8 *get_region(int flags);
void machine_init();
void machine_frame();
void machine_processsound(void *data, int len);

#endif
