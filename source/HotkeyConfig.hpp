#pragma once
#include <windows.h>
#include <string>
#include <cwctype>
#include <cstdlib>
namespace gt4 {
struct HotkeyConfig{int vk=VK_F1;bool ctrl=false,invalid=false,migrated=false;};
inline int parseOpenKey(std::wstring value){
 const auto first=value.find_first_not_of(L" \t\r\n"),last=value.find_last_not_of(L" \t\r\n");if(first==std::wstring::npos)return 0;
 value=value.substr(first,last-first+1);for(auto&c:value)c=wchar_t(std::towupper(c));
 if(value.size()==1&&((value[0]>=L'A'&&value[0]<=L'Z')||(value[0]>=L'0'&&value[0]<=L'9')))return int(value[0]);
 auto number=[](const wchar_t* text,int base){wchar_t* end=nullptr;const long n=std::wcstol(text,&end,base);return end!=text&&!*end&&n>=1&&n<=255?int(n):0;};
 if(value[0]==L'F'&&value.size()>1){const int n=number(value.c_str()+1,10);return n>=1&&n<=24?VK_F1+n-1:0;}
 if(value==L"INSERT")return VK_INSERT;if(value==L"HOME")return VK_HOME;if(value==L"END")return VK_END;if(value==L"TAB")return VK_TAB;
 return number(value.c_str(),value.size()>2&&value[0]==L'0'&&value[1]==L'X'?16:10);
}
inline HotkeyConfig readHotkey(const std::wstring& ini){HotkeyConfig result;wchar_t text[80]{};GetPrivateProfileStringW(L"GT4Showroom",L"OpenKey",L"F1",text,80,ini.c_str());const int vk=parseOpenKey(text);result.invalid=!vk;if(vk)result.vk=vk;return result;}
}
