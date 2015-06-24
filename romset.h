#ifndef __romset_h__
#define __romset_h__

typedef struct rom_s {
	char *filename;
	int loadaddr;
	int size;
	int crc32;
	int flags;
} rom_t;

typedef struct romset_s {
	char *name;
	rom_t roms[];
} romset_t;

extern romset_t r_1942;

int romset_load(romset_t *romset, char *path);

#endif
