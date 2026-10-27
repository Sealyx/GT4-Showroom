#pragma once
#include <windows.h>
#include <string>
namespace gt4 {
struct SecretFeatures {bool enabled=true;};
inline SecretFeatures readSecretFeatures(const std::wstring&ini){return {GetPrivateProfileIntW(L"SecretFeatures",L"Enabled",1,ini.c_str())!=0};}
}
