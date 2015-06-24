#include <SDL/SDL.h>
#include "romset.h"
#include "machine.h"
#include "system.h"
#include "../deadz80/deadz80.h"

int main(int argc, char *argv[])
{
	screenw = 256;
	screenh = 256;
	screenbpp = 32;

	system_init();
	video_init();

	machine_init();

	if (romset_load(&r_1942, ".\\1942") != 0) {
		return(0);
	}

	decode_palette();


	while (quit == 0) {
		system_checkevents();
		system_poll();
		machine_frame();
		system_frame();
	}

	video_kill();
	system_kill();
	return(0);
}
