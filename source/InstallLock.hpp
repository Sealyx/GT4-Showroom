#pragma once
#include <windows.h>
#include <cstdio>
#include <cwchar>
namespace modding {
struct InstallLock {
    HANDLE mutexes[2]{};
    unsigned held=0;
    bool acquired=false;
    InstallLock() {
        const wchar_t* formats[2]={L"Local\\GTA-SA-Native-Hook-Install-%lu",L"Local\\GTA-SA-Codex-Hook-Install-%lu"};
        for(unsigned i=0;i<2;++i) {
            wchar_t name[80]{};
            std::swprintf(name,80,formats[i],GetCurrentProcessId());
            mutexes[i]=CreateMutexW(nullptr,FALSE,name);
            if(!mutexes[i])return;
            const DWORD result=WaitForSingleObject(mutexes[i],5000);
            if(result!=WAIT_OBJECT_0 && result!=WAIT_ABANDONED)return;
            ++held;
        }
        acquired=true;
    }
    ~InstallLock() {
        for(unsigned i=2;i>0;--i)if(mutexes[i-1]) {
            if(i<=held)ReleaseMutex(mutexes[i-1]);
            CloseHandle(mutexes[i-1]);
        }
    }
    InstallLock(const InstallLock&)=delete;
    InstallLock& operator=(const InstallLock&)=delete;
};
}
