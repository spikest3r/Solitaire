#include "res.h";
__declspec(noinline) Resource::Resource(const char* dllName, bool* result) {
	hDll = LoadLibraryA(dllName);
	if (!hDll) {
		MessageBoxA(NULL, "Failed to load resource DLL!", "Fatal engine error", MB_OK | MB_ICONWARNING);
		if(result != nullptr) *result = false;
		return;
	}
	*result = true;
}

void* Resource::loadResource(int resId, LPCWSTR lpType, DWORD* size) {
	HRSRC hRes = FindResource(hDll, MAKEINTRESOURCE(resId), lpType);
	if (!hRes) {
		return nullptr;
	}

	if(size) *size = SizeofResource(hDll, hRes);

	HGLOBAL hData = LoadResource(hDll, hRes);
	if (!hData) {
		return nullptr;
	}

	return LockResource(hData);
}