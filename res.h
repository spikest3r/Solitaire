#pragma once
#include "Windows.h"
class Resource {
public:
	Resource(const char* dllName, bool* result);
	HINSTANCE hDll;
	void* loadResource(int resId, LPCWSTR lpType, DWORD* size);
};