#pragma once
#include "ModelFilter.hpp"
#include <array>
#include <windows.h>
namespace gt4 {
struct SecretUnlocksConfig {
 ExclusionConfig models;bool enableModkit=true,enablePaintjob=false,enableAbilities=true;
 SecretUnlocksConfig(){parseModelTokens("calica",models);}
 template<class E>bool matches(const E& entry)const{return models.hides(entry);}
};
inline SecretUnlocksConfig readSecretUnlocks(const std::wstring& path){
 SecretUnlocksConfig result;result.models={};std::array<wchar_t,32768> buffer{};
 const DWORD n=GetPrivateProfileStringW(L"SecretUnlocks",L"Model",L"calica",buffer.data(),DWORD(buffer.size()),path.c_str());
 std::string text;for(DWORD i=0;i<n;++i)text+=buffer[i]<128?char(buffer[i]):'?';parseModelTokens(text,result.models);result.models.invalid+=n>=buffer.size()-1;
 result.enableModkit=GetPrivateProfileIntW(L"SecretUnlocks",L"EnableModkit",1,path.c_str())!=0;
 result.enablePaintjob=GetPrivateProfileIntW(L"SecretUnlocks",L"EnablePaintjob",0,path.c_str())!=0;
 result.enableAbilities=GetPrivateProfileIntW(L"SecretUnlocks",L"EnableAbilities",1,path.c_str())!=0;return result;
}
}
