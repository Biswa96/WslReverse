#define UNICODE
#include <Windows.h>

BOOL ExtractResource(LPCWSTR ResName, LPCWSTR FileName) {
    HMODULE hModule = LoadLibraryExW(L"lxss\\LxssManager.dll", 0, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HRSRC hResInfo = FindResourceW(hModule, ResName, RT_RCDATA);
    HGLOBAL hResData = LoadResource(hModule, hResInfo);
    HANDLE lpBuffer = LockResource(hResData);
    DWORD dwSize = SizeofResource(hModule, hResInfo);
    HANDLE hFile = CreateFileW(FileName, GENERIC_ALL, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    BOOL result = WriteFile(hFile, lpBuffer, dwSize, 0, 0);
    return result;
}

int main() {
    ExtractResource(L"#2", L"init");
    ExtractResource(L"#3", L"bsdtar");
    ExtractResource(L"#4", L"init.cpio.gz");
    return 0;
}
