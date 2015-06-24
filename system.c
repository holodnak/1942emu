#include <windows.h>
#include <SDL/SDL.h>
#include "system.h"
#include "sound.h"
#include "../deadz80/deadz80.h"

SDL_Surface *surface = 0;
int quit = 0;
int flags = SDL_DOUBLEBUF | SDL_HWSURFACE;
int screenw, screenh, screenbpp;
unsigned char screen[256 * (256 + 32)];
unsigned int palette[256];
unsigned char joykeys[370];
double interval;
unsigned __int64 t, lasttime;

int system_init()
{
	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK)) {
		printf("error at sdl init!\n");
		return(1);
	}

	interval = (double)system_getfrequency() / 60.0f;
	lasttime = system_gettick();

	if (sound_init() != 0) {
		return(1);
	}

	sound_play();

	return(0);
}

void system_kill()
{
	sound_kill();
	SDL_Quit();
}

void system_checkevents()
{
	static int keydown = 0;
	SDL_Event event; /* Event structure */

	while (SDL_PollEvent(&event)) {
		switch (event.type) {
		case SDL_QUIT:
			quit++;
			break;
		case SDL_KEYDOWN:
		case SDL_KEYUP:
			break;
		}
	}
}

void system_poll()
{
	u8 *keystate = SDL_GetKeyState(NULL);
	int i;

	for (i = 0; i<308; i++)
		joykeys[i] = keystate[i];

	if (joykeys[SDLK_ESCAPE])
		quit++;
}

void system_frame()
{
	int x, y;
	u8 *src = screen;
	u32 *dest;


	u8 rot[256 * 256], *rotd;

	rotd = rot;
	rotd += (256 - 1) * 256;
	for (y = 0; y < 256; y++) {
		for (x = 0; x < 256; x++) {
			rotd[x] = src[x * 256];
		}
		rotd -= 256;
		src++;
	}
	src = rot;

	SDL_LockSurface(surface);

	dest = (u32*)surface->pixels;

	for (y = 0; y < 256; y++) {
		for (x = 16; x < 240; x++) {
			u32 pixel = palette[src[x]];

			dest[x * 2 + 0] = pixel;
			dest[x * 2 + 1] = pixel;
			dest[x * 2 + 0 + 512] = pixel;
			dest[x * 2 + 1 + 512] = pixel;
		}
		src += 256;
		dest += surface->pitch / 4 * 2;
	}

	SDL_Flip(surface);
	SDL_UnlockSurface(surface);

	if (joykeys[SDLK_F8] == 0) {
		do {
			t = system_gettick();
		} while ((double)(t - lasttime) < interval);
	}
	lasttime = t;
}

unsigned __int64 system_gettick()
{
	LARGE_INTEGER li;

	QueryPerformanceCounter(&li);
	return(li.QuadPart);
}

unsigned __int64 system_getfrequency()
{
	LARGE_INTEGER li;

	if (QueryPerformanceFrequency(&li) == 0)
		return(1);
	return(li.QuadPart);
}

int video_init()
{
	surface = SDL_SetVideoMode(screenw * 2, screenh * 2, screenbpp, flags);
	SDL_WM_SetCaption("1942emu", NULL);
	SDL_ShowCursor(0);
	return(0);
}

void video_kill()
{
	SDL_ShowCursor(1);
}
