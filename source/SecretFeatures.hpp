#pragma once
#include <windows.h>
#include <string>
namespace gt4 {
struct SecretFeatures {bool licenseFail=true,wastedBusted=true;};
inline SecretFeatures readSecretFeatures(const std::wstring&ini){return {
 GetPrivateProfileIntW(L"SecretFeatures",L"LicenseFail",1,ini.c_str())!=0,
 GetPrivateProfileIntW(L"SecretFeatures",L"WastedBusted",1,ini.c_str())!=0};}
}
