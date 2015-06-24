#ifndef AY8910_H
#define AY8910_H

#define MAX_8910 4

#define AY8910Write ay8910_write_ym

#define NULL 0

typedef unsigned char UINT8, BYTE;
typedef unsigned short UINT16;
typedef unsigned int UINT32;
typedef signed char INT8;
typedef signed short INT16;
typedef signed int INT32;

void _AYWriteReg(int n, int r, int v);
void ay8910_write_ym(int chip, int addr, int data);
void AY8910_reset(int chip);
//void AY8910Update(int chip,INT16 **buffer,int length);
void AY8910Update(int chip,INT16 *buf1,INT16 *buf2,INT16 *buf3,int length);

void AY8910_InitAll(int nClock, int nSampleRate);
void AY8910_InitClock(int nClock);
BYTE* AY8910_GetRegsPtr(int nAyNum);

#endif
