#ifndef __system_h__
#define __system_h__

extern int quit;
extern int screenw, screenh, screenbpp;
extern unsigned char screen[];
extern unsigned int palette[];
extern unsigned char joykeys[];

int system_init();
void system_kill();
void system_checkevents();
void system_poll();
void system_frame();
unsigned __int64 system_gettick();
unsigned __int64 system_getfrequency();
int video_init();
void video_kill();

#endif
