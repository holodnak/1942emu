#include <SDL/SDL.h>
#include "machine.h"
#include "system.h"
#include "sound.h"
#include "ay8910/ay8910.h"
#include "../deadz80/deadz80.h"

//1942 hardware
static deadz80_t maincpu, soundcpu;
static u8 rom[0x20000];					//main cpu rom
static u8 soundrom[0x4000];				//sound cpu rom
static u8 chars[0x2000];				//characters
static u8 tiles[0xC000];				//tiles
static u8 sprites[0x10000];				//sprites
static u8 prom[0x1000];					//proms
static u8 ram[0x1000];					//main cpu ram
static u8 fgram[0x800], bgram[0x400];	//fg and bg character ram
static u8 sprram[0x80];					//sprite ram
static u8 soundram[0x800];				//ram for sound cpu
static u8 bankswitch, *bankptr;			//bankswitch stuff
static u8 palettebank;					//palette bank select
static u16 scroll;						//scroll register
static u8 soundlatch;					//sound latch byte
static u8 input[8];						//input + dip
static u8 bglookup[0x400];
static u8 sprlookup[0x100];
static u8 charlookup[0x100];

void decode_palette()
{
	int i;
	unsigned char b0, b1, b2, b3;
	unsigned char r, g, b;

	//create the palette
	for (i = 0; i<256; i++) {

		b0 = ((prom[i] >> 0) & 1) * 0x0E;
		b1 = ((prom[i] >> 1) & 1) * 0x1F;
		b2 = ((prom[i] >> 2) & 1) * 0x43;
		b3 = ((prom[i] >> 3) & 1) * 0x8F;
		r = b0 + b1 + b2 + b3;

		b0 = ((prom[i + 256] >> 0) & 1) * 0x0E;
		b1 = ((prom[i + 256] >> 1) & 1) * 0x1F;
		b2 = ((prom[i + 256] >> 2) & 1) * 0x43;
		b3 = ((prom[i + 256] >> 3) & 1) * 0x8F;
		g = b0 + b1 + b2 + b3;

		b0 = ((prom[i + 512] >> 0) & 1) * 0x0E;
		b1 = ((prom[i + 512] >> 1) & 1) * 0x1F;
		b2 = ((prom[i + 512] >> 2) & 1) * 0x43;
		b3 = ((prom[i + 512] >> 3) & 1) * 0x8F;
		b = b0 + b1 + b2 + b3;

		palette[i] = (r << 16) | (g << 8) | b;
	}

	for (i = 0; i<256; i++) {
		bglookup[i] = 0x00 | prom[i + 0x400];
		bglookup[i + 256] = 0x10 | prom[i + 0x400];
		bglookup[i + 512] = 0x20 | prom[i + 0x400];
		bglookup[i + 768] = 0x30 | prom[i + 0x400];
		sprlookup[i] = 0x40 | prom[i + 0x500];
		charlookup[i] = 0x80 | prom[i + 0x300];
	}
}

u8 maincpu_read(u32 addr)
{
	if (addr < 0x8000) {
		return(rom[addr]);
	}
	if (addr < 0xC000) {
		return(bankptr[addr & 0x3FFF]);
	}
	switch (addr & 0xF000) {
	case 0xC000:
		switch (addr) {
		case 0xC000:
		case 0xC001:
		case 0xC002:
		case 0xC003:
		case 0xC004:
			return(input[addr & 7]);
		}
		break;
	case 0xD000:
		if (addr < 0xD800) {
			return(fgram[addr & 0x7FF]);
		}
		if (addr < 0xDC00) {
			return(bgram[addr & 0x3FF]);
		}
		break;
	case 0xE000:
		return(ram[addr & 0xFFF]);
	}
	printf("maincpu_read:  unhandled read at $%04X\n", addr);
	return(0);
}

void maincpu_write(u32 addr, u8 data)
{
	switch (addr & 0xF000) {
	case 0xC000:
		if (addr >= 0xCC00 && addr < 0xCC80) {
			sprram[addr & 0x7F] = data;
			return;
		}
		switch (addr) {
		case 0xC800:
			soundlatch = data;
			return;
		case 0xC802:
			scroll = (scroll & 0xFF00) | data;
			return;
		case 0xC803:
			scroll = (scroll & 0x00FF) | (data << 8);
			return;
		case 0xC804:
			//reset sound cpu
			if (data & 0x10) {
				deadz80_setcontext(&soundcpu);
				deadz80_reset();
				deadz80_setcontext(&maincpu);
			}
			//flipscreen = data & 0x80;
			return;
		case 0xC805:
			palettebank = data;
			return;
		case 0xC806:
			bankswitch = data;
			bankptr = (u8*)rom + 0x10000 + (data * 0x4000);
			return;
		}
		break;
	case 0xD000:
		if (addr < 0xD800) {
			fgram[addr & 0x7FF] = data;
			return;
		}
		if (addr < 0xDC00) {
			bgram[addr & 0x3FF] = data;
			return;
		}
		break;
	case 0xE000:
		ram[addr & 0xFFF] = data;
		return;
	}
	printf("maincpu_write:  unhandled write at $%04X = $%02X\n", addr, data);
}

u8 soundcpu_read(u32 addr)
{
	if (addr < 0x4000) {
		return(soundrom[addr]);
	}
	if (addr < 0x4800) {
		return(soundram[addr & 0x7FF]);
	}
	if (addr == 0x6000) {
		return(soundlatch);
	}
	printf("soundcpu_read:  unhandled read at $%04X\n", addr);
	return(0);
}

void soundcpu_write(u32 addr, u8 data)
{
	if (addr >= 0x4000 && addr < 0x4800) {
		soundram[addr & 0x7FF] = data;
		return;
	}

	switch (addr) {
	case 0x8000:
	case 0x8001:
		AY8910Write(0, addr & 1, data);
		return;
	case 0xC000:
	case 0xC001:
		AY8910Write(1, addr & 1, data);
		return;
	}
	printf("soundcpu_write:  unhandled write at $%04X = $%02X\n", addr, data);
}

u8 mainirq(u8 state)
{
	deadz80_clear_irq(state);
	if (state == 1)
		return(0xD7);
	return(0xCF);
}

u8 soundirq(u8 state)
{
	deadz80_clear_irq(state);
	return(0xFF);
}

u8 *get_region(int flags)
{
	switch (flags & 0xFF) {
	case CPU0: return(rom);
	case CPU1: return(soundrom);
	case GFX0: return(chars);
	case GFX1: return(tiles);
	case GFX2: return(sprites);
	case PROM0:return(prom);
	}
	return(0);
}

static void draw_fgtile(unsigned char *dest, unsigned char tile, unsigned char attr)
{
	int x, y;
	int index = (tile | ((attr & 0x80) << 1)) * 16;
	u8 pixel;
	u8 *tiledata = &chars[index];

	attr <<= 2;
	for (y = 0; y < 8; y++) {
		for (x = 3; x >= 0; x--) {
			pixel = (tiledata[0] >> (x + 4)) & 1;
			pixel |= ((tiledata[0] >> (x + 0)) & 1) << 1;
			if (pixel) {
				dest[(3 - x) + 0] = charlookup[pixel | attr];
			}
			pixel = (tiledata[1] >> (x + 4)) & 1;
			pixel |= ((tiledata[1] >> (x + 0)) & 1) << 1;
			if (pixel) {
				dest[(3 - x) + 4] = charlookup[pixel | attr];
			}
		}
		tiledata += 2;
		dest += 256;
	}
}

static void draw_bgtile(u8 *dest, int idx)
{
	int x, y, dy, inc;
	u8 pixel;
	int index = (idx & 0x0F) | ((idx & 0x1F0) << 1);
	u32 tile = bgram[index];
	u32 attr = bgram[index + 16];
	u8 *tiledata0, *tiledata1, *tiledata2;
	u8 flipx, flipy;
	u8 tilebuf[16][16];

	tiledata0 = &tiles[tile * 32];
	if (attr & 0x80) {
		tiledata0 += 0x6000;
	}
	tiledata1 = tiledata0 + 0x2000;
	tiledata2 = tiledata0 + 0x4000;

	flipy = attr & 0x40;
	flipx = attr & 0x20;

	attr = ((attr & 0x1F) | ((palettebank & 3) << 5)) << 3;

	if (flipy) {
		dy = 15;
		inc = -1;
	}
	else {
		dy = 0;
		inc = 1;
	}

	//draw tile to buffer
	for (y = 0; y < 16; y++) {
		for (x = 7; x >= 0; x--) {
			pixel = ((tiledata0[0] >> x) & 1) << 2;
			pixel |= ((tiledata1[0] >> x) & 1) << 1;
			pixel |= ((tiledata2[0] >> x) & 1) << 0;
			tilebuf[dy][(7 - x) + 0] = bglookup[pixel | attr];
			pixel = ((tiledata0[16] >> x) & 1) << 2;
			pixel |= ((tiledata1[16] >> x) & 1) << 1;
			pixel |= ((tiledata2[16] >> x) & 1) << 0;
			tilebuf[dy][(7 - x) + 8] = bglookup[pixel | attr];
		}
		tiledata0 += 1;
		tiledata1 += 1;
		tiledata2 += 1;
		dy += inc;
	}

	//copy tile buffer to screen
	if (flipx) {
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 16; x++) {
				dest[y + x * 256] = tilebuf[y][15 - x];
			}
		}
	}
	else {
		for (y = 0; y < 16; y++) {
			for (x = 0; x < 16; x++) {
				dest[y + x * 256] = tilebuf[y][x];
			}
		}
	}
}

static u8 bgscreen[512 * 512];

static void draw_bg()
{
	int x, y, idx;
	int sx, sy;

	//draw all tiles to temporary screen
	for (idx = 0, y = 0; y < 32; y++) {
		for (x = 0; x < 16; x++) {
			draw_bgtile((u8*)bgscreen + (x * 16) + (y * 256 * 16), idx++);
		}
	}

	//copy the visible area to the real screen
	for (y = 0; y < 256; y++) {
		for (x = 0; x < 256; x++) {
			sx = x;
			sy = ((y + scroll) & 0x1FF);
			screen[y + x * 256] = bgscreen[sx + sy * 256];
		}
	}
}

static void draw_fg()
{
	int x, y;
	u8 *src = fgram;

	for (y = 0; y < 32; y++) {
		for (x = 0; x < 32; x++) {
			draw_fgtile(&screen[(x * 8) + (y * 8 * 256)], src[x], src[x + 0x400]);
		}
		src += 32;
	}
}

static void draw_sprtile(int tile, u8 attr, int sx, int sy)
{
	int x, y;
	u8 *dest = &screen[sx + sy * 256];
	int index = tile * 64;
	u8 pixel;
	u8 *tiledata0 = &sprites[index];
	u8 *tiledata1 = tiledata0 + 0x8000;
	u8 line[16];

	for (y = 0; y < 16; y++) {
		for (x = 3; x >= 0; x--) {
			pixel = ((tiledata0[0] >> (x + 4)) & 1) << 0;
			pixel |= ((tiledata0[0] >> (x + 0)) & 1) << 1;
			pixel |= ((tiledata1[0] >> (x + 4)) & 1) << 2;
			pixel |= ((tiledata1[0] >> (x + 0)) & 1) << 3;
			line[(3 - x) + 0] = sprlookup[pixel | attr];

			pixel = ((tiledata0[1] >> (x + 4)) & 1) << 0;
			pixel |= ((tiledata0[1] >> (x + 0)) & 1) << 1;
			pixel |= ((tiledata1[1] >> (x + 4)) & 1) << 2;
			pixel |= ((tiledata1[1] >> (x + 0)) & 1) << 3;
			line[(3 - x) + 4] = sprlookup[pixel | attr];

			pixel = ((tiledata0[32] >> (x + 4)) & 1) << 0;
			pixel |= ((tiledata0[32] >> (x + 0)) & 1) << 1;
			pixel |= ((tiledata1[32] >> (x + 4)) & 1) << 2;
			pixel |= ((tiledata1[32] >> (x + 0)) & 1) << 3;
			line[(3 - x) + 8] = sprlookup[pixel | attr];

			pixel = ((tiledata0[33] >> (x + 4)) & 1) << 0;
			pixel |= ((tiledata0[33] >> (x + 0)) & 1) << 1;
			pixel |= ((tiledata1[33] >> (x + 4)) & 1) << 2;
			pixel |= ((tiledata1[33] >> (x + 0)) & 1) << 3;
			line[(3 - x) + 12] = sprlookup[pixel | attr];
		}
		for (x = 0; x < 16; x++) {
			if ((sx + x) >= 256) {
				break;
			}
			if ((line[x] & 0xF) != 0xF) {
				dest[x] = line[x];
			}
		}
		tiledata0 += 2;
		tiledata1 += 2;
		dest += 256;
	}

}

static void draw_sprites()
{
	int offs, i, code, sx, sy, ypos;
	u8 attr;

	for (offs = 0x80 - 4; offs >= 0; offs -= 4) {
		code = (sprram[offs] & 0x7F) | ((sprram[offs] & 0x80) << 1) | ((sprram[offs + 1] & 0x20) << 2);
		attr = sprram[offs + 1] & 0x0F;
		sx = sprram[offs + 3] - ((sprram[offs + 1] & 0x10) << 4);
		sy = sprram[offs + 2];

		i = (sprram[offs + 1] & 0xC0) >> 6;
		i = (i == 2) ? 3 : i;
		do {
			draw_sprtile(code + i, attr << 4, sx, sy + 16 * i);
			i--;
		} while (i >= 0);
	}
}

static void draw_frame()
{
	draw_bg();
	draw_sprites();
	draw_fg();
}

void machine_reset()
{
	deadz80_setcontext(&maincpu);
	deadz80_reset();
	deadz80_setcontext(&soundcpu);
	deadz80_reset();

	AY8910_reset(0);
	AY8910_reset(1);

	bankswitch = 0;
	bankptr = (u8*)rom + 0x10000;
	scroll = 0;
	soundlatch = 0;
}

void machine_init()
{
	int i;

	for (i = 0; i < 5; i++) {
		input[i] = 0xFF;
	}

	//dip switches
	input[3] = 0x37;
	input[4] = 0x57 | 0x80 | 0x08;

	memset(&maincpu, 0, sizeof(deadz80_t));
	memset(&soundcpu, 0, sizeof(deadz80_t));

	strcpy(maincpu.tag, "main");
	strcpy(soundcpu.tag, "sound");
	deadz80_init();

	AY8910_InitClock(1500000);
	AY8910_InitAll(1500000, 44100);	//2x 1.5mhz, 44100hz output

	for (i = 0; i < Z80_NUMPAGES; i++) {
		maincpu.readfuncs[i] = maincpu_read;
		maincpu.writefuncs[i] = maincpu_write;
		soundcpu.readfuncs[i] = soundcpu_read;
		soundcpu.writefuncs[i] = soundcpu_write;
	}
	maincpu.irqfunc = mainirq;
	soundcpu.irqfunc = soundirq;

//	sound_setcallback(machine_processsound);

	machine_reset();
}

void machine_processsound(void *data,int len)
{
	INT16 *dest = (INT16*)data;
	INT16 b1[3][1024];
	INT16 b2[3][1024];
	int n;

	//	printf("processing %d bytes\n", len);
	AY8910Update(0, (INT16*)b1[0], (INT16*)b1[1], (INT16*)b1[2], len);
	AY8910Update(1, (INT16*)b2[0], (INT16*)b2[1], (INT16*)b2[2], len);

	for (n = 0; n<len; n++)
		dest[n] = (b1[0][n] + b1[1][n] + b1[2][n] + b2[0][n] + b2[1][n] + b2[2][n]) / 6;

	sound_update(dest, len);
}

void machine_frame()
{
	int i;
	int mainlinecycles = 12000000 / 3 / 57 / 272;
	int soundlinecycles = 12000000 / 4 / 57 / 272;

	input[0] = input[1] = input[2] = 0;

	if (joykeys[SDLK_1])			input[0] |= 0x01;
	if (joykeys[SDLK_2])			input[0] |= 0x02;
	if (joykeys[SDLK_5])			input[0] |= 0x40;
	if (joykeys[SDLK_6])			input[0] |= 0x80;

	if (joykeys[SDLK_RIGHT])	input[1] |= 0x01;
	if (joykeys[SDLK_LEFT])		input[1] |= 0x02;
	if (joykeys[SDLK_DOWN])		input[1] |= 0x04;
	if (joykeys[SDLK_UP])		input[1] |= 0x08;
	if (joykeys[SDLK_LCTRL])	input[1] |= 0x10;
	if (joykeys[SDLK_LALT])		input[1] |= 0x20;

	input[0] ^= 0xFF;
	input[1] ^= 0xFF;
	input[2] ^= 0xFF;

	//run for one frame of 272 scanlines
	for (i = 0; i < 272; i++) {
		deadz80_setcontext(&maincpu);
		deadz80_execute(mainlinecycles * 4);
		if (i == 0) {
			deadz80_set_irq(2);
			deadz80_irq();
		}
		if (i == 240) {
			deadz80_set_irq(1);
			deadz80_irq();
		}

		deadz80_setcontext(&soundcpu);
		deadz80_execute(soundlinecycles * 4);
		if (i == 10 || i == 90 || i == 170 || i == 250) {
			deadz80_irq();
		}
	}
	draw_frame();
	{
		INT16 data[1024];
		machine_processsound(data,44100 / 60);
	}
}
