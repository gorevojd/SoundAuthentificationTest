#ifndef PLATFORM_HPP
#include <Windows.h>

#pragma pack(push, 1)
struct WAVHEADER
{
	DWORD chunkName;		/*"RIFF" 0x52494646*/
	DWORD chunkSize;		/*== 36 + dataChunkSize. –азмер цепочки, начина€ с этой позиции.*/
	DWORD waveChunk;		/*"WAVE" 0x57415645*/

	DWORD fmtChunkName;		/*"fmt " 0x666d7420*/
	DWORD fmtChunkSize;		//==16
	WORD  format;			/*==WAVE_FORMAT_PCM - линейное квантование. «начени€, отличающиес€ от 1, обозначают некоторый формат сжати€.*/
	WORD  channels;
	DWORD samplesPerSecond;
	DWORD avgBytesPerSec;	/*==samplesPerSecond * numChannels * bitsPerSample/8*/
	WORD  blockAlign;		/*==numChannels * bitsPerSample/8*/
	WORD  bitsPerSample;	//==16

	DWORD dataChunkName;	/*"data" 0x64617461*/
	DWORD dataChunkSize;	/*==nSamplesPerSec*nChannels*nBitsPerSample/8*nSeconds*/
};
#pragma pack(pop)

struct loaded_wav_sound{
	WAVHEADER header;
	BYTE* data;
	DWORD dataSize;
};

struct get_sound_samples_result{
	void* memory;
	DWORD size;
};

struct read_file_result{
	void* memory;
	DWORD size;
};


void MyWriteFile(const char* name, void* MemoryToWrite, DWORD Size);
read_file_result MyReadFile(const char* name);

get_sound_samples_result GetSoundSamples(
	void* Region1,
	DWORD Region1Size,
	void* Region2,
	DWORD Region2Size);
void FreeGetSoundSamplesResult(get_sound_samples_result* result);

#define PLATFORM_HPP
#endif