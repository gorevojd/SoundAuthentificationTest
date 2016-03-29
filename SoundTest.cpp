#include <Windows.h>
#include <dsound.h>
#include "SoundTest.h"
#include "platform.h"

#include "resource.h"

static bool GlobalRunning;
static screen_buffer* globalScreen = new screen_buffer;
static sound_output* globalSoundOutput = new sound_output;
static sound_buffer globalSoundBuffer = {};
static capture_buffer globalSoundCaptureBuffer = {};

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPGUID, LPDIRECTSOUND*, LPUNKNOWN)
typedef DIRECT_SOUND_CREATE(direct_sound_create);
DIRECT_SOUND_CREATE(direct_sound_create_stub){
	return 0;
}
static direct_sound_create* DirectSoundCreate_ = direct_sound_create_stub;
#define DirectSoundCreate DirectSoundCreate_

#define DIRECT_SOUND_CAPTURE_CREATE(name) HRESULT WINAPI name(LPCGUID , LPDIRECTSOUNDCAPTURE*, LPUNKNOWN)
typedef DIRECT_SOUND_CAPTURE_CREATE(direct_sound_capture_create);
DIRECT_SOUND_CAPTURE_CREATE(DirectSoundCaptureCreate_stub){
	return 0;
}
static direct_sound_capture_create* DirectSoundCaptureCreate_ = DirectSoundCaptureCreate_stub;
#define DirectSoundCaptureCreate DirectSoundCaptureCreate_

void GetWindowDimension(HWND hWnd, i32* width, i32* height)
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	i32 w = rect.right - rect.left;
	i32 h = rect.bottom - rect.top;
	*width = w;
	*height = h;
}

loaded_wav_sound ReadWAVFile(const char* name)
{
	read_file_result ReadResult = MyReadFile(name);
	WAVHEADER* header = (WAVHEADER*)ReadResult.memory;
	BYTE* Data = (BYTE*)ReadResult.memory + 44;

	loaded_wav_sound Result;
	Result.header = *header;
	Result.data = Data;
	Result.dataSize = header->dataChunkSize;

	return Result;
}

void WriteSoundFileFromSoundBuffer(const char* name, LPDIRECTSOUNDBUFFER Buffer)
{
	get_sound_samples_result gssResult;
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;

	HRESULT LockResult = Buffer->Lock(
		0, 0,
		&Region1, &Region1Size,
		&Region2, &Region2Size,
		DSBLOCK_ENTIREBUFFER);

	gssResult = GetSoundSamples(
		Region1, Region1Size,
		Region2, Region2Size);

	HRESULT UnlockResult = Buffer->Unlock(
		Region1,
		Region1Size,
		Region2,
		Region2Size);

	DWORD FileSize = sizeof(WAVHEADER) + gssResult.size;
	void* WriteBuffer = VirtualAlloc(
		0,
		FileSize,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);

	WAVHEADER* header = new WAVHEADER;
	header->dataChunkSize = gssResult.size;
	header->chunkName = 0x46464952;
	header->chunkSize = 36 + header->dataChunkSize;
	header->waveChunk = 0x45564157;
	header->fmtChunkName = 0x20746d66;
	header->fmtChunkSize = 16;
	header->format = WAVE_FORMAT_PCM;
	header->channels = 2;
	header->samplesPerSecond = 44100;
	header->bitsPerSample = 16;
	header->blockAlign = 4;
	header->avgBytesPerSec = 44100 * 4;
	header->dataChunkName = 0x61746164;
	WriteBuffer = header;

	//TODO:
	HANDLE FileHandle = CreateFileA(
		name,
		GENERIC_WRITE,
		0,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);
	DWORD BytesWritten;
	if (FileHandle != INVALID_HANDLE_VALUE){
		WriteFile(FileHandle,
			WriteBuffer,
			sizeof(WAVHEADER),
			&BytesWritten,
			0);
		char str[10];
		_itoa_s(BytesWritten, str, 10);
		OutputDebugStringA(str);
		OutputDebugStringA("\n");
		BytesWritten = 0;
		WriteFile(FileHandle,
			gssResult.memory,
			gssResult.size,
			&BytesWritten,
			0);
		_itoa_s(BytesWritten, str, 10);
		OutputDebugStringA(str);
		OutputDebugStringA("\n");
		CloseHandle(FileHandle);
	}
	else{

	}
	FreeGetSoundSamplesResult(&gssResult);
}



capture_buffer win32InitDirectSoundCapture()
{
	/*
		if (SUCCEEDED(hr = pDSC->CreateCaptureBuffer(&dscbd, &pDSCB, NULL)))
		{
		  hr = pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)ppDSCB8);
		  pDSCB->Release();  
		}
		return hr;
	*/

	HINSTANCE DSoundLib = LoadLibraryA("dsound.dll");
	direct_sound_capture_create* DirectSoundCaptureCreate =
		(direct_sound_capture_create*)GetProcAddress(DSoundLib, "DirectSoundCaptureCreate");

	LPDIRECTSOUNDCAPTURE DirectSoundCapture;
	HRESULT CaptureCreateResult = DirectSoundCaptureCreate(0, &DirectSoundCapture, 0);
	if (DirectSoundCaptureCreate && SUCCEEDED(CaptureCreateResult))
	{
		WAVEFORMATEX WaveFormat = {};
		WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		WaveFormat.nChannels = 2;
		WaveFormat.nSamplesPerSec = 44100;
		WaveFormat.wBitsPerSample = 16;
		WaveFormat.nBlockAlign = 4;
		WaveFormat.nAvgBytesPerSec = 44100 * 4;
		WaveFormat.cbSize = 0;

		DSCBUFFERDESC CaptureBufferDescription = {};
		CaptureBufferDescription.dwSize = sizeof(DSCBUFFERDESC);
		CaptureBufferDescription.dwFlags = 0;
		CaptureBufferDescription.dwBufferBytes = WaveFormat.nAvgBytesPerSec * 4;
		CaptureBufferDescription.dwReserved = 0;
		CaptureBufferDescription.lpwfxFormat = &WaveFormat;
		CaptureBufferDescription.dwFXCount = 0;
		CaptureBufferDescription.lpDSCFXDesc = NULL;
		
		capture_buffer result;
		if (SUCCEEDED(DirectSoundCapture->CreateCaptureBuffer(
			&CaptureBufferDescription,
			&result.buffer, 0)))
		{
			result.avgBytesPerSec = WaveFormat.nAvgBytesPerSec;
			result.bitsPerSample = WaveFormat.wBitsPerSample;
			result.blockAlign = WaveFormat.nBlockAlign;
			result.channels = WaveFormat.nChannels;
			result.format = WaveFormat.wFormatTag;
			result.samplesPerSecond = WaveFormat.nSamplesPerSec;
			result.size = WaveFormat.nAvgBytesPerSec * 4; // 1 second lenght
			return result;
		}
	}
	else{
		if (CaptureCreateResult == DSERR_INVALIDPARAM){
			OutputDebugStringA("DSCaptureCreate Error: DSERR_INVALIDPARAM\n");
		}
		if (CaptureCreateResult == DSERR_NOAGGREGATION){
			OutputDebugStringA("DSCaptureCreate Error: DSERR_NOAGGREGATION\n");
		}
		if (CaptureCreateResult == DSERR_OUTOFMEMORY){
			OutputDebugStringA("DSCaptureCreate Error: DSERR_OUTOFMEMORY\n");
		}
	}
}

sound_buffer win32InitDirectSound(HWND Window)
{
	HINSTANCE DSoundLib = LoadLibraryA("dsound.dll");
	direct_sound_create* DirectSoundCreate =
		(direct_sound_create*)GetProcAddress(DSoundLib, "DirectSoundCreate");

	LPDIRECTSOUND DirectSound;
	if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
		WAVEFORMATEX WaveFormat = {};
		WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		WaveFormat.nChannels = 2;
		WaveFormat.nSamplesPerSec = 44100;
		WaveFormat.wBitsPerSample = 16;
		WaveFormat.nBlockAlign = 4;
		WaveFormat.nAvgBytesPerSec = 44100 * 4;
		WaveFormat.cbSize = sizeof(WAVEFORMATEX);
		
		if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
		{	
			DSBUFFERDESC Description = {};
			Description.dwSize = sizeof(DSBUFFERDESC);
			Description.dwFlags = DSBCAPS_PRIMARYBUFFER;
			Description.dwBufferBytes = 0;
			Description.dwReserved = 0;
			Description.guid3DAlgorithm = GUID_NULL;
			Description.lpwfxFormat = 0;
			
			LPDIRECTSOUNDBUFFER PrimaryBuffer;
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Description, &PrimaryBuffer, 0)))
			{
				PrimaryBuffer->SetFormat(&WaveFormat);
			}
			else{
				OutputDebugStringA("Can not create primary buffer\n");
			}
		}
		else{
			OutputDebugStringA("Can not set coop level\n");
		}

		DSBUFFERDESC Desc = {};
		Desc.dwSize = sizeof(DSBUFFERDESC);
		Desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
		Desc.dwBufferBytes = WaveFormat.nAvgBytesPerSec * 4; // 4 sesond lenght buffer
		Desc.lpwfxFormat = &WaveFormat;
		Desc.dwReserved = 0;
		Desc.guid3DAlgorithm = GUID_NULL;

		LPDIRECTSOUNDBUFFER SecondaryBuffer;
		if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Desc, &SecondaryBuffer, 0)))
		{
			sound_buffer result;
			result.buffer = SecondaryBuffer;
			result.format = WaveFormat.wFormatTag;
			result.avgBytesPerSec = WaveFormat.nAvgBytesPerSec;
			result.bitsPerSample = WaveFormat.wBitsPerSample;
			result.channels = WaveFormat.nChannels;
			result.size = Desc.dwBufferBytes;
			result.samplesPerSecond = WaveFormat.nSamplesPerSec;
			result.runningSample = 0;
			return result;
		}
		else
		{
			OutputDebugStringA("Can not create secondary buffer\n");
		}
	}
	else
	{
		OutputDebugStringA("Can not create direct sound object\n");
	}
}

sound_buffer CreateSoundBufferForLoadedSound(HWND Window, loaded_wav_sound sound)
{
	HINSTANCE DSoundLib = LoadLibraryA("dsound.dll");
	direct_sound_create* DirectSoundCreate =
		(direct_sound_create*)GetProcAddress(DSoundLib, "DirectSoundCreate");
	
	LPDIRECTSOUND DirectSound;
	if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
	{
		WAVEFORMATEX WaveFormat = {};
		//TODO(Dima): Change this if other sound format.
		WaveFormat.wFormatTag = sound.header.format;
		WaveFormat.nChannels = sound.header.channels;
		WaveFormat.nSamplesPerSec = sound.header.samplesPerSecond;
		WaveFormat.wBitsPerSample = sound.header.bitsPerSample;
		WaveFormat.nBlockAlign = sound.header.blockAlign;
		WaveFormat.nAvgBytesPerSec = sound.header.avgBytesPerSec;
		WaveFormat.cbSize = sizeof(WAVEFORMATEX);

		if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
		{
			DSBUFFERDESC Description = {};
			Description.dwSize = sizeof(DSBUFFERDESC);
			Description.dwFlags = DSBCAPS_PRIMARYBUFFER;
			Description.dwBufferBytes = 0;
			Description.dwReserved = 0;
			Description.guid3DAlgorithm = GUID_NULL;
			Description.lpwfxFormat = 0;

			LPDIRECTSOUNDBUFFER PrimaryBuffer;
			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Description, &PrimaryBuffer, 0)))
			{
				PrimaryBuffer->SetFormat(&WaveFormat);
			}
			else{
				OutputDebugStringA("Can not create primary buffer\n");
			}
		}
		else{
			OutputDebugStringA("Can not set coop level\n");
		}

		DSBUFFERDESC Desc = {};
		Desc.dwSize = sizeof(DSBUFFERDESC);
		Desc.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
		Desc.dwBufferBytes = sound.dataSize;
		Desc.lpwfxFormat = &WaveFormat;
		Desc.dwReserved = 0;
		Desc.guid3DAlgorithm = GUID_NULL;

		sound_buffer result;
		if (SUCCEEDED(DirectSound->CreateSoundBuffer(&Desc, &result.buffer, 0)))
		{
			result.avgBytesPerSec = sound.header.avgBytesPerSec;
			result.bitsPerSample = sound.header.bitsPerSample;
			result.blockAlign = sound.header.blockAlign;
			result.channels = sound.header.channels;
			result.format = sound.header.format;
			result.samplesPerSecond = sound.header.samplesPerSecond;
			result.size = Desc.dwBufferBytes;
			result.runningSample = 0;
			
			DWORD WriteCursor;
			DWORD PlayCursor;

			HRESULT GetPosResult =
				result.buffer->GetCurrentPosition(
				&PlayCursor,
				&WriteCursor);
			if (SUCCEEDED(GetPosResult)){

				void* Region1;
				DWORD Region1Size;
				void* Region2;
				DWORD Region2Size;

				HRESULT LockResult =
					result.buffer->Lock(
					0, result.size,
					&Region1, &Region1Size,
					&Region2, &Region2Size,
					0);

				if (SUCCEEDED(LockResult)){

					DWORD Region1SampleCount = Region1Size / result.blockAlign;
					DWORD Region2SampleCount = Region2Size / result.blockAlign;

					u16* sOut = (u16*)Region1;
					u16* sIn = (u16*)sound.data;

					for (DWORD s = 0;
						s < Region1SampleCount;
						s++)
					{
						*sOut++ = *sIn++;
						*sOut++ = *sIn++;
					}

					for (DWORD s = 0;
						s < Region2SampleCount;
						s++)
					{
						*sOut++ = *sIn++;
						*sOut++ = *sIn++;
					}

					HRESULT UnlockResult =
						result.buffer->Unlock(
						Region1,
						Region1Size,
						Region2,
						Region2Size);
					if (!SUCCEEDED(UnlockResult)){
						OutputDebugStringA("Can not unlock buffer.\n");
					}
				}
				else{
					OutputDebugStringA("Can not lock buffer\n");
					if (LockResult == DSERR_BUFFERLOST){
						OutputDebugStringA("DSERR_BUFFERLOST\n");
					}
					if (LockResult == DSERR_INVALIDCALL){
						OutputDebugStringA("DSERR_INVALIDCALL\n");
					}
					if (LockResult == DSERR_INVALIDPARAM){
						OutputDebugStringA("DSERR_INVALIDPARAM\n");
					}
					if (LockResult == DSERR_PRIOLEVELNEEDED){
						OutputDebugStringA("DSERR_PRIOLEVELNEEDED\n");
					}
				}
			}
			else{
				OutputDebugStringA("Can not get current position of cursors.\n");
			}

			return result;
		}
		else
		{
			OutputDebugStringA("Can not create secondary buffer\n");
		}
	}

}

void FillSoundBufferWithCapturedData(sound_buffer& sBuffer, capture_buffer& cBuffer){
	cBuffer.runningSample = 0;
	sBuffer.runningSample = 0;
	DWORD& cRunningSample = cBuffer.runningSample;
	DWORD& sRunningSample = sBuffer.runningSample;

	DWORD captureCursor;
	DWORD readCursor;
	if (DS_OK == cBuffer.buffer->GetCurrentPosition(&captureCursor, &readCursor)){

		DWORD captureByte = (cRunningSample * cBuffer.blockAlign) % cBuffer.size;
		DWORD bytesToWrite;

#if 1
		if (captureByte == 0){
			bytesToWrite = cBuffer.size;
		}
		else{
			if (captureByte > readCursor){
				bytesToWrite = cBuffer.size - captureByte + readCursor;
			}
			else{
				bytesToWrite = readCursor - captureByte;
			}
		}
#else
		if (captureCursor == 0){
			bytesToWrite = cBuffer.size;
		}
		else{
			if (captureCursor > readCursor){
				bytesToWrite = cBuffer.size - captureCursor + readCursor;
			}
			else{
				bytesToWrite = readCursor - captureCursor;
			}
		}
#endif

		void* capturedRegion1;
		DWORD capturedRegion1Size;
		void* capturedRegion2;
		DWORD capturedRegion2Size;

		HRESULT captureLockResult =
			cBuffer.buffer->Lock(
			captureByte,
			bytesToWrite,
			&capturedRegion1,
			&capturedRegion1Size,
			&capturedRegion2,
			&capturedRegion2Size,
			0);

		if (SUCCEEDED(captureLockResult)){
			//TODO(Dima): Write to the sound buffer

			get_sound_samples_result gssResult = GetSoundSamples(
				capturedRegion1, capturedRegion1Size,
				capturedRegion2, capturedRegion2Size);

			DWORD capturedRegion1SampleCount = capturedRegion1Size / cBuffer.blockAlign;
			DWORD capturedRegion2SampleCount = capturedRegion2Size / cBuffer.blockAlign;

			HRESULT UnlockResult =
				cBuffer.buffer->Unlock(
				capturedRegion1,
				capturedRegion1Size,
				capturedRegion2,
				capturedRegion2Size);

			if (SUCCEEDED(UnlockResult)){
				DWORD writeCursor;
				DWORD playCursor;
				sBuffer.buffer->GetCurrentPosition(&playCursor, &writeCursor);

				DWORD tByteToWrite = (sRunningSample * sBuffer.blockAlign) % sBuffer.size;
				DWORD tBytesToWrite;

				if (captureByte == 0){
					tBytesToWrite = cBuffer.size;
				}
				else{
					if (tByteToWrite > playCursor){
						tBytesToWrite = cBuffer.size - tByteToWrite + playCursor;
					}
					else{
						tBytesToWrite = playCursor - tByteToWrite;
					}
				}

				void* tRegion1;
				DWORD tRegion1Size;
				void* tRegion2;
				DWORD tRegion2Size;

				HRESULT LockResult2 =
					sBuffer.buffer->Lock(
					captureByte,
					bytesToWrite,
					&tRegion1,
					&tRegion1Size,
					&tRegion2,
					&tRegion2Size,
					0);

				if (SUCCEEDED(LockResult2)){			
					sRunningSample = 0;
					u16* sourceSample = (u16*)gssResult.memory;
					u16* targetSample = (u16*)tRegion1;
					DWORD tRegion1SampleCount = tRegion1Size / sBuffer.blockAlign;
					DWORD tRegion2SampleCount = tRegion2Size / sBuffer.blockAlign;

					for (DWORD s = 0;
						s < capturedRegion1SampleCount;
						s++)
					{
						*targetSample++ = *sourceSample++;
						*targetSample++ = *sourceSample++;
						sRunningSample++;
					}

					for (DWORD s = 0;
						s < capturedRegion2SampleCount;
						s++)
					{
						*targetSample++ = *sourceSample++;
						*targetSample++ = *sourceSample++;
						sRunningSample++;
					}

					HRESULT UnlockResult2 =
						sBuffer.buffer->Unlock(
						tRegion1,
						tRegion1Size,
						tRegion2,
						tRegion2Size);
				}

			}
			else{
				OutputDebugStringA("Can not unlock capture buffer.\n");
			}

		}
		else{
			OutputDebugStringA("Can not lock capture buffer\n");
			if (captureLockResult == DSERR_BUFFERLOST){
				OutputDebugStringA("DSERR_BUFFERLOST\n");
			}
			if (captureLockResult == DSERR_INVALIDCALL){
				OutputDebugStringA("DSERR_INVALIDCALL\n");
			}
			if (captureLockResult == DSERR_INVALIDPARAM){
				OutputDebugStringA("DSERR_INVALIDPARAM\n");
			}
			if (captureLockResult == DSERR_PRIOLEVELNEEDED){
				OutputDebugStringA("DSERR_PRIOLEVELNEEDED\n");
			}
		}
	}
	else{
		OutputDebugStringA("Can not get current position in capture buffer.\n");
	}
}

void FillSoundBuffer(sound_buffer sBuffer, sound_output* sOutput, i32 freq){
	sOutput->freq = freq;
	r32 wavePeriod = 1.0f / sOutput->freq;
	DWORD PeriodSamples = wavePeriod * 44100;
	DWORD HalfPeriodSamples = PeriodSamples / 2;
	DWORD PeriodBytes = PeriodSamples * 4; // * 2 channels * 2 bytes per sample
	i16 Volume = 1000;

	DWORD& RunSample = sBuffer.runningSample;
	RunSample = RunSample % PeriodSamples;

	DWORD WriteCursor;
	DWORD PlayCursor;
	if (SUCCEEDED(sBuffer.buffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
	{
		DWORD BytesToLock;
		DWORD ByteToLock;

		ByteToLock = RunSample * 4 % sBuffer.size;

		if (PlayCursor == ByteToLock)
		{
			BytesToLock = sBuffer.size;
		}
		else{
			if (PlayCursor < ByteToLock){
				BytesToLock = sBuffer.size - (ByteToLock - PlayCursor);
			}
			else{
				BytesToLock = PlayCursor - ByteToLock;
			}
		}

		LPVOID region1ptr;
		DWORD region1Size;
		LPVOID region2ptr;
		DWORD region2Size;

		HRESULT Result =
			sBuffer.buffer->Lock(
			ByteToLock,
			BytesToLock,
			&region1ptr, &region1Size,
			&region2ptr, &region2Size,
			0);

		if (SUCCEEDED(Result))
		{
			//OutputDebugStringA("Buffer locked succesfully\n");
			DWORD region1SampleCount = region1Size / 4;
			DWORD region2SampleCount = region2Size / 4;

			i16* sOut = (i16*)region1ptr;
			for (DWORD sIndex = 0;
				sIndex < region1SampleCount;
				sIndex++)
			{
				i16 SampleOut = (RunSample % PeriodSamples > HalfPeriodSamples) ? Volume : -Volume;
				*sOut++ = SampleOut;
				*sOut++ = SampleOut;
				RunSample++;
			}

			sOut = (i16*)region2ptr;
			for (DWORD sIndex = 0;
				sIndex < region2SampleCount;
				sIndex++)
			{
				i16 SampleOut = (RunSample % PeriodSamples > HalfPeriodSamples) ? Volume : -Volume;
				*sOut++ = SampleOut;
				*sOut++ = SampleOut;
				RunSample++;
			}
			
			if (SUCCEEDED(sBuffer.buffer->Unlock(
				region1ptr, region1Size,
				region2ptr, region2Size)))
			{
				
			}
			else{
				OutputDebugStringA("Can not unlock buffer\n");
			}
		}
		else{
			OutputDebugStringA("Can not lock buffer\n");
			if (Result == DSERR_BUFFERLOST){
				OutputDebugStringA("DSERR_BUFFERLOST\n");
			}
			if (Result == DSERR_INVALIDCALL){
				OutputDebugStringA("DSERR_INVALIDCALL\n");
			}
			if (Result == DSERR_INVALIDPARAM){
				OutputDebugStringA("DSERR_INVALIDPARAM\n");
			}
			if (Result == DSERR_PRIOLEVELNEEDED){
				OutputDebugStringA("DSERR_PRIOLEVELNEEDED\n");
			}
		}
	}
	else{
		OutputDebugStringA("Can not get current position\n");
	}
}

void StartCaptureBuffer(LPDIRECTSOUNDCAPTUREBUFFER Buffer){
	HRESULT Result = Buffer->Start(DSCBSTART_LOOPING);
	if (SUCCEEDED(Result))
	{
		
	}
	else{
		if (Result == DSERR_BUFFERLOST){
			OutputDebugStringA("DSERR_BUFFERLOST\n");
		}
		if (Result == DSERR_INVALIDCALL){
			OutputDebugStringA("DSERR_INVALIDCALL\n");
		}
		if (Result == DSERR_INVALIDPARAM){
			OutputDebugStringA("DSERR_INVALIDPARAM\n");
		}
		if (Result == DSERR_PRIOLEVELNEEDED){
			OutputDebugStringA("DSERR_PRIOLEVELNEEDED\n");
		}
	}
}

void StopCaptureBuffer(LPDIRECTSOUNDCAPTUREBUFFER Buffer){
	Buffer->Stop();
}

void PlaySoundBuffer(LPDIRECTSOUNDBUFFER Buffer)
{
	HRESULT Result = Buffer->Play(0, 0, DSBPLAY_LOOPING);
	if (SUCCEEDED(Result))
	{
		//OutputDebugStringA("Start playing sound succesfully!\n");
	}
	else{
		if (Result == DSERR_BUFFERLOST){
			OutputDebugStringA("DSERR_BUFFERLOST\n");
		}
		if (Result == DSERR_INVALIDCALL){
			OutputDebugStringA("DSERR_INVALIDCALL\n");
		}
		if (Result == DSERR_INVALIDPARAM){
			OutputDebugStringA("DSERR_INVALIDPARAM\n");
		}
		if (Result == DSERR_PRIOLEVELNEEDED){
			OutputDebugStringA("DSERR_PRIOLEVELNEEDED\n");
		}
	}
}

void StopSoundBuffer(LPDIRECTSOUNDBUFFER Buffer)
{
	Buffer->Stop();
}

void win32InitScreenBuffer(screen_buffer* Buffer, i32 width, i32 height){
	if (Buffer->Memory){
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	
	BITMAPINFO bmi = {};
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = -height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = 0;
	bmi.bmiHeader.biXPelsPerMeter = 0;
	bmi.bmiHeader.biYPelsPerMeter = 0;
	bmi.bmiHeader.biClrUsed = 0;
	bmi.bmiHeader.biClrImportant = 0;
	Buffer->bmi = bmi;

	Buffer->width = width;
	Buffer->height = height;

	int size = width * height * 4;
	Buffer->Memory = 
		VirtualAlloc(0, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

void win32DisplayMemoryToWindow(HDC dc, screen_buffer* Buffer, i32 width, i32 height)
{
#if 0
	StretchDIBits(dc, 
		0, 0, width, height,
		0, 0, Buffer->width, Buffer->height,
		Buffer->Memory, &Buffer->bmi,
		DIB_RGB_COLORS, SRCCOPY);
#else
	StretchDIBits(dc,
		0, 0, Buffer->width, Buffer->height,
		0, 0, Buffer->width, Buffer->height,
		Buffer->Memory, &Buffer->bmi,
		DIB_RGB_COLORS, SRCCOPY);
#endif
}

void RenderMovingGradient(screen_buffer* Buffer, float& offset){
	for (int j = 0; j < Buffer->height; j++){
		for (int i = 0; i < Buffer->width; i++){
			u8 red = i * 2 + offset;
			u8 green = j * 2;
			u32 color = ((red << 16) ^ (green << 8)) ^ 0xAAAAAAAA ^ 0xDDDDDDD;
			u32* Pixel = (u32*)Buffer->Memory + j*Buffer->width + i;
			*Pixel = color;
		}
	}
	offset += 0.5f;
}


LRESULT CALLBACK
WndProc(HWND Window,
	UINT Message,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (Message){
	case WM_CLOSE:{
		GlobalRunning = false;
	}break;
	case WM_DESTROY:{
		GlobalRunning = false;
	}break;	
	case WM_CREATE:{

	}break;
	case WM_PAINT:{
		PAINTSTRUCT Paint;
		HDC hdc = BeginPaint(Window, &Paint);
		i32 width = globalScreen->width;
		i32 height = globalScreen->height;
		i32 w, h;
		GetWindowDimension(Window, &w, &h);
		EndPaint(Window, &Paint);
	}break;
	case WM_SIZE:{
	}break;
	case WM_MOUSEMOVE:{

	}break;
	default:{
		return DefWindowProc(Window, Message, wParam, lParam);
	}break;
	}
	return 0;
}



int WINAPI 
WinMain(HINSTANCE Instance, 
	HINSTANCE PrevInstance, 
	LPSTR ComandLine,
	int ComandShow)
{
	win32InitScreenBuffer(globalScreen, 640, 480);

	WNDCLASSEX WindowClass = {};
	WindowClass.cbSize = sizeof(WNDCLASSEX);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.hInstance = Instance;
	WindowClass.lpfnWndProc = WndProc;
	WindowClass.lpszClassName = L"WindowClassName";
	WindowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WindowClass.hIcon = LoadIcon(Instance, MAKEINTRESOURCE(IDI_APPLICATION));
	WindowClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	LoadMenu(Instance, WindowClass.lpszMenuName);

	RegisterClassEx(&WindowClass);

	globalScreen->Window =
		CreateWindowEx(0, WindowClass.lpszClassName,
		L"SoundTest", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		50, 50,
		globalScreen->width,
		globalScreen->height,
		0, 0, Instance, 0);

	LPDIRECTSOUNDBUFFER lpdsBuffer;
	globalSoundBuffer =	win32InitDirectSound(globalScreen->Window);
	globalSoundCaptureBuffer = win32InitDirectSoundCapture();

	loaded_wav_sound loadedMusic = ReadWAVFile("music.wav");
	sound_buffer musicBuffer =
		CreateSoundBufferForLoadedSound(globalScreen->Window, loadedMusic);

	FillSoundBuffer(globalSoundBuffer, globalSoundOutput, 256);
	WriteSoundFileFromSoundBuffer("MusicTest.wav", globalSoundBuffer.buffer);

	float gradOffset = 0;
	bool capturingSound = false;
	GlobalRunning = true;
	while (GlobalRunning == true){
		MSG Message;
		while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&Message);
			DispatchMessage(&Message);
			if (Message.message == WM_QUIT){
				GlobalRunning = false;
			}
			
			else if (Message.message == WM_COMMAND){
				switch (LOWORD(Message.wParam)){
					/*
					#define IDR_MENU1                       101
					#define ID_TEST_SOUNDBUFFER             40001
					#define ID_TEST_CAPTUREBUFFER           40002
					#define ID_SOUNDBUFFER_PLAY             40003
					#define ID_SOUNDBUFFER_STOP             40004
					#define ID_CAPTUREBUFFER_START          40005
					#define ID_CAPTUREBUFFER_STOP           40006
					#define ID_CAPTUREBUFFER_COPYTOSOUNDBUFFER 40007
					#define ID_TEST_MUSIC                   40008
					#define ID_MUSIC_PLAY                   40009
					#define ID_MUSIC_STOP                   40010
					*/
				case ID_SOUNDBUFFER_PLAY:{
					PlaySoundBuffer(globalSoundBuffer.buffer);
				}break;
				case ID_SOUNDBUFFER_STOP:{
					StopSoundBuffer(globalSoundBuffer.buffer);
				}break;
				case ID_CAPTUREBUFFER_START:{
					StartCaptureBuffer(globalSoundCaptureBuffer.buffer);
				}break;
				case ID_CAPTUREBUFFER_STOP:{
					StopCaptureBuffer(globalSoundCaptureBuffer.buffer);
				}break;
				case ID_CAPTUREBUFFER_COPYTOSOUNDBUFFER:{
					FillSoundBufferWithCapturedData(globalSoundBuffer, globalSoundCaptureBuffer); 
				}break;
				case ID_MUSIC_PLAY:{
					PlaySoundBuffer(musicBuffer.buffer);
				}break;
				case ID_MUSIC_STOP:{
					StopSoundBuffer(musicBuffer.buffer);
				}break;
				}
			}
			
		}
		
		HDC dc = GetDC(globalScreen->Window);
		RenderMovingGradient(globalScreen, gradOffset);
		i32 w, h;
		GetWindowDimension(globalScreen->Window, &w, &h);
		win32DisplayMemoryToWindow(dc, globalScreen, w, h);
		ReleaseDC(globalScreen->Window, dc);
	}

	PostQuitMessage(0);
	return 0;
}