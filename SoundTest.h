#ifndef SOUND_TEST

#include <Windows.h>
#include <dsound.h>
#include "Common.h"

struct screen_buffer{
	i32 width;
	i32 height;
	HWND Window;
	void* Memory;
	BITMAPINFO bmi;
};

struct sound_output{
	i32 freq;
};

struct sound_buffer{
	LPDIRECTSOUNDBUFFER buffer;
	DWORD size;
	WORD  format;
	WORD  channels;
	DWORD samplesPerSecond;
	DWORD avgBytesPerSec;	
	WORD  blockAlign;	
	WORD  bitsPerSample;
	DWORD runningSample;
};

struct capture_buffer{
	LPDIRECTSOUNDCAPTUREBUFFER buffer;
	DWORD size;
	WORD  format;
	WORD channels;
	DWORD samplesPerSecond;
	DWORD avgBytesPerSec;
	WORD  blockAlign;
	WORD  bitsPerSample;
	DWORD runningSample;
};

#define SOUND_TEST
#endif