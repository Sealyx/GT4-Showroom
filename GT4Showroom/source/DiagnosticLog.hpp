#pragma once
#include <windows.h>
#include <atomic>
#include <cstdio>
#include <string>
struct DiagnosticLog {
 std::atomic<bool> enabled{false};
 void reload(const std::wstring& ini,const wchar_t* section){enabled.store(GetPrivateProfileIntW(section,L"EnableLog",0,ini.c_str())==1,std::memory_order_relaxed);}
 void write(const std::wstring& path,const char* message) const {
  if(!enabled.load(std::memory_order_relaxed))return;
  FILE* file=nullptr;_wfopen_s(&file,path.c_str(),L"ab");
  if(file){std::fprintf(file,"[%llu] %s\r\n",static_cast<unsigned long long>(GetTickCount64()),message);std::fclose(file);}
 }
};
