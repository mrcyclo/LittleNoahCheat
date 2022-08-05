#pragma once
#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <psapi.h>
#include "kiero/kiero.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include <vector>

typedef HRESULT(__stdcall* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);
typedef uintptr_t PTR;

char* ScanBasic(const char* pattern, const char* mask, char* begin, intptr_t size)
{
	intptr_t patternLen = strlen(mask);

	//for (int i = 0; i < size - patternLen; i++)
	for (int i = 0; i < size; i++)
	{
		bool found = true;
		for (int j = 0; j < patternLen; j++)
		{
			if (mask[j] != '?' && pattern[j] != *(char*)((intptr_t)begin + i + j))
			{
				found = false;
				break;
			}
		}
		if (found)
		{
			return (begin + i);
		}
	}
	return nullptr;
}

char* ScanPattern(HMODULE hModule, const char* pattern, const char* mask)
{
	char* match{ nullptr };
	DWORD oldprotect = 0;
	MEMORY_BASIC_INFORMATION mbi{};

	MODULEINFO modinfo = { 0 };
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(modinfo));

	char* begin = (char*)hModule;
	for (char* curr = begin; curr < begin + modinfo.SizeOfImage; curr += mbi.RegionSize)
	{
		if (!VirtualQuery(curr, &mbi, sizeof(mbi)) || mbi.State != MEM_COMMIT || mbi.Protect == PAGE_NOACCESS) continue;

		if (VirtualProtect(mbi.BaseAddress, mbi.RegionSize, PAGE_EXECUTE_READWRITE, &oldprotect))
		{
			match = ScanBasic(pattern, mask, curr, mbi.RegionSize);

			VirtualProtect(mbi.BaseAddress, mbi.RegionSize, oldprotect, &oldprotect);

			if (match != nullptr)
			{
				break;
			}
		}
	}
	return match;
}

void MemPatch(BYTE* dst, BYTE* src, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);

	memcpy(dst, src, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void MemPatchEx(BYTE* dst, BYTE* src, unsigned int size, HANDLE hProcess)
{
	DWORD oldprotect;
	VirtualProtectEx(hProcess, dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	WriteProcessMemory(hProcess, dst, src, size, nullptr);
	VirtualProtectEx(hProcess, dst, size, oldprotect, &oldprotect);
}

void MemNop(BYTE* dst, unsigned int size)
{
	DWORD oldprotect;
	VirtualProtect(dst, size, PAGE_EXECUTE_READWRITE, &oldprotect);
	memset(dst, 0x90, size);
	VirtualProtect(dst, size, oldprotect, &oldprotect);
}

void MemNopEx(BYTE* dst, unsigned int size, HANDLE hProcess)
{
	BYTE* nopArray = new BYTE[size];
	memset(nopArray, 0x90, size);

	MemPatchEx(dst, nopArray, size, hProcess);
	delete[] nopArray;
}

bool IsBadReadPtr(void* p)
{
	MEMORY_BASIC_INFORMATION mbi{};
	if (VirtualQuery(p, &mbi, sizeof(mbi)))
	{
		DWORD mask = (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY);
		bool b = !(mbi.Protect & mask);
		// check the page is not a guard page
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) b = true;
		return b;
	}
	return true;
}

uintptr_t* MemFindDMAAddy(uintptr_t* ptr, std::vector<unsigned int> offsets)
{
	if (ptr == nullptr) return nullptr;

	uintptr_t* addr = ptr;
	for (unsigned int i = 0; i < offsets.size(); ++i)
	{
		addr = (uintptr_t*)*addr;
		if (addr == nullptr) return nullptr;
		if (IsBadReadPtr(addr)) return nullptr;

		addr = reinterpret_cast<uintptr_t*>(reinterpret_cast<char*>(addr) + offsets[i]);
		if (IsBadReadPtr(addr)) return nullptr;
	}
	return addr;
}

struct List__Array {
	void* klass; //0x0
	void* monitor; //0x8
	void* bounds; //0x10
	uintptr_t max_length; //0x18
	uintptr_t vector[32]; //0x20
};

struct List__Fields {
	struct List__Array* _items; //0x10
	int32_t _size; //0x18
};

struct List {
	void* klass; //0x0
	void* monitor; //0x8
	struct List__Fields fields; //0x10
};
