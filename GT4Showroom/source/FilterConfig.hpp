#pragma once
#include <windows.h>
#include "ModelFilter.hpp"
namespace gt4 {
inline ExclusionConfig readExclusions(const std::wstring& ini){ExclusionConfig result;
 auto read=[&](const wchar_t* key,const wchar_t* fallback){std::vector<wchar_t> text(32768);const auto n=GetPrivateProfileStringW(L"ExcludedModels",key,fallback,text.data(),DWORD(text.size()),ini.c_str());if(n>=text.size()-1)++result.invalid;std::string value;for(auto c:text){if(!c)break;value+=c<128?char(c):'?';}return value;};
 const auto unified=read(L"ExcludedModels",L"\x1");if(unified!="\x1"){parseModelTokens(unified,result);return result;}
 // Legacy groups remain readable; the unified key takes precedence, even empty.
 for(size_t i=0;i<6;++i){const std::string key=filterKeys[i],value=filterDefaults[i];const std::wstring wkey(key.begin(),key.end()),fallback(value.begin(),value.end());parseModelTokens(read(wkey.c_str(),fallback.c_str()),result);}return result;
}
}
