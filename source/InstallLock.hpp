#pragma once
#include <windows.h>
#include <cstdio>
#include <cwchar>
namespace modding {
// Coordinate shared engine entries between these independently loaded ASIs.
// Always acquire outside DllMain, before creating any trampoline.
struct InstallLock {
    HANDLE mutex=nullptr;
    bool acquired=false;
    InstallLock() {
        wchar_t name[80];
        std::swprintf(name,80,L"Local\\GTA-SA-Native-Hook-Install-%lu",GetCurrentProcessId());
        mutex=CreateMutexW(nullptr,FALSE,name);
        if(mutex) {
            const DWORD result=WaitForSingleObject(mutex,5000);
            acquired=result==WAIT_OBJECT_0 || result==WAIT_ABANDONED;
        }
    }
    ~InstallLock() { if(mutex) { if(acquired) ReleaseMutex(mutex); CloseHandle(mutex); } }
    InstallLock(const InstallLock&)=delete;
    InstallLock& operator=(const InstallLock&)=delete;
};
}
