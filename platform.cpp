#include <Windows.h>
#include "platform.h"

void FreeGetSoundSamplesResult(get_sound_samples_result* result){
	VirtualFree(result->memory, result->size, MEM_RELEASE);
}

get_sound_samples_result GetSoundSamples(
	void* Region1,
	DWORD Region1Size,
	void* Region2,
	DWORD Region2Size)
{
	DWORD size = Region1Size + Region2Size;
	VOID* ResultMemory = VirtualAlloc(
		0,
		size,
		MEM_COMMIT | MEM_RESERVE,
		PAGE_READWRITE);
	WORD* ReadSample = (WORD*)ResultMemory;

	WORD* sOut = (WORD*)Region1;
	for (DWORD s = 0; s < Region1Size / 4; s++){
		*ReadSample++ = *sOut++;
		*ReadSample++ = *sOut++;
	}

	sOut = (WORD*)Region2;
	for (DWORD s = 0; s < Region2Size / 4; s++){
		*ReadSample++ = *sOut++;
		*ReadSample++ = *sOut++;
	}
	get_sound_samples_result Result;
	Result.memory = ResultMemory;
	Result.size = Region1Size + Region2Size;

	return Result;
}

read_file_result MyReadFile(const char* name)
{
	read_file_result Result = {};
	HANDLE handle = CreateFileA(
		name,
		GENERIC_READ,
		0,
		0,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (handle != INVALID_HANDLE_VALUE){
		LARGE_INTEGER FileSize;
		GetFileSizeEx(handle, &FileSize);
		BYTE* ResultMemory = new BYTE[FileSize.QuadPart];
		DWORD BytesRead;
		if (ReadFile(handle, ResultMemory, FileSize.QuadPart, &BytesRead, 0)){
			CloseHandle(handle);
			char str[10];
			_itoa_s(BytesRead, str, 10);
			OutputDebugStringA(str);
			Result.memory = ResultMemory;
			Result.size = FileSize.QuadPart;
			return Result;
		}
		else{
			CloseHandle(handle);
			OutputDebugStringA("Can not read file.\n");
		}
	}
	else{
		OutputDebugStringA("Can not create file when reading.\n");
	}
	return Result;
}

void MyWriteFile(const char* name, void* MemoryToWrite, DWORD Size)
{
	HANDLE handle = CreateFileA(
		name,
		GENERIC_WRITE,
		0,
		0,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		0);

	if (handle != INVALID_HANDLE_VALUE){
		DWORD BytesWritten;
		BOOL WriteResult =
			WriteFile(handle, MemoryToWrite, Size, &BytesWritten, 0);
		if (WriteResult == TRUE){

		}
		else{
			DWORD Error = GetLastError();
			if (Error == ERROR_INVALID_USER_BUFFER){
				OutputDebugStringA("Can not write file: ERROR_INVALID_USER_BUFFER.\n");
			}
			if (Error == ERROR_NOT_ENOUGH_MEMORY){
				OutputDebugStringA("Can not write file: ERROR_NOT_ENOUGH_MEMORY.\n");
			}
		}
	}
	else{
		OutputDebugStringA("Can not create file when writing.\n");
	}
}