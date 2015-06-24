#include <stdio.h>
#include "romset.h"
#include "machine.h"

romset_t r_1942 = {
	"1942",
	{
		//main cpu roms
		{ "srb-03.m3", 0x00000, 0x4000, 0xd9dafcc3, CPU0 },
		{ "srb-04.m4", 0x04000, 0x4000, 0xda0cf924, CPU0 },
		{ "srb-05.m5", 0x10000, 0x4000, 0xd102911c, CPU0 },
		{ "srb-06.m6", 0x14000, 0x2000, 0x466f8248, CPU0 },
		{ "srb-07.m7", 0x18000, 0x4000, 0x0d31038c, CPU0 },

	//sound cpu roms
		{ "sr-01.c11", 0x0000, 0x4000, 0xbd87f06b, CPU1 },

		{ "sr-02.f2", 0x0000, 0x2000, 0x6ebca191, GFX0 },    /* characters */

		{ "sr-08.a1", 0x0000, 0x2000, 0x3884d9eb, GFX1 },    /* tiles */
		{ "sr-10.a3", 0x2000, 0x2000, 0x8edb273a, GFX1 },
		{ "sr-12.a5", 0x4000, 0x2000, 0x1bd3d8bb, GFX1 },
		{ "sr-09.a2", 0x6000, 0x2000, 0x999cf6e0, GFX1 },
		{ "sr-11.a4", 0x8000, 0x2000, 0x3a2726c3, GFX1 },
		{ "sr-13.a6", 0xa000, 0x2000, 0x658f02c4, GFX1 },

		{ "sr-14.l1", 0x00000, 0x4000, 0x2528bec6, GFX2 },   /* sprites */
		{ "sr-15.l2", 0x04000, 0x4000, 0xf89287aa, GFX2 },
		{ "sr-16.n1", 0x08000, 0x4000, 0x024418f8, GFX2 },
		{ "sr-17.n2", 0x0c000, 0x4000, 0xe2c7e489, GFX2 },

		{ "sb-5.e8",  0x0000, 0x0100, 0x93ab8153, PROM0 },    /* red component */
		{ "sb-6.e9",  0x0100, 0x0100, 0x8ab44f7d, PROM0 },    /* green component */
		{ "sb-7.e10", 0x0200, 0x0100, 0xf4ade9a4, PROM0 },    /* blue component */
		{ "sb-0.f1",  0x0300, 0x0100, 0x6047d91b, PROM0 },    /* char lookup table */
		{ "sb-4.d6",  0x0400, 0x0100, 0x4858968d, PROM0 },    /* tile lookup table */
		{ "sb-8.k3",  0x0500, 0x0100, 0xf6fad943, PROM0 },    /* sprite lookup table */
		{ "sb-2.d1",  0x0600, 0x0100, 0x8bb8b3df, PROM0 },    /* tile palette selector? (not used) */
		{ "sb-3.d2",  0x0700, 0x0100, 0x3b0c99af, PROM0 },    /* tile palette selector? (not used) */
		{ "sb-1.k6",  0x0800, 0x0100, 0x712ac508, PROM0 },    /* interrupt timing (not used) */
		{ "sb-9.m11", 0x0900, 0x0100, 0x4921635c, PROM0 },    /* video timing? (not used) */
		{ 0,0,0,0,0 }
	}
};

int romset_load(romset_t *romset, char *path)
{
	rom_t *rom;				//currently being loaded rom
	int i, n;
	FILE *fp;
	char fn[1024];
	u8 *ptr;

	for (i = 0;; i++) {
		rom = &romset->roms[i];
		if (rom->filename == 0)
			break;
		sprintf(fn, "%s\\%s", path, rom->filename);
		if ((fp = fopen(fn, "rb")) == 0) {
			printf("error opening rom '%s'\n", fn);
			return(1);
		}
		ptr = get_region(rom->flags);
		fread(ptr + rom->loadaddr, rom->size, 1, fp);
		fclose(fp);
		printf("loaded '%s' at $%05X\n", rom->filename, rom->loadaddr);
	}

	return(0);
}
