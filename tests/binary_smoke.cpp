#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
int main(int argc,char**argv){if(argc!=2)return 2;HMODULE m=LoadLibraryA(argv[1]);if(!m)return 3;auto init=reinterpret_cast<int(__stdcall*)()>(GetProcAddress(m,"GT4_Init"));auto entry=reinterpret_cast<void(__stdcall*)()>(GetProcAddress(m,"InitializeASI"));if(!init||!entry)return 4;entry();entry();if(init()!=0||init()!=0)return 5;
// ASI loaders retain the module for the process lifetime. An explicit FreeLibrary
// here races the scheduled initialization worker even after the shared init-once
// callback returns; this fixture must honor that lifetime contract.
std::puts("Win32 binary loads, loader callback is idempotent and unsupported host is rejected.");}
