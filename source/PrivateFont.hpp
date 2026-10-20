#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <string>
namespace gt4 {
class PrivateFont {
 std::wstring file,selected=L"Arial";bool owned=false;
public:
 ~PrivateFont(){unload();}
 PrivateFont()=default;PrivateFont(const PrivateFont&)=delete;PrivateFont&operator=(const PrivateFont&)=delete;
 bool load(const std::wstring&path,const std::wstring&family){unload();if(path.empty()||family.empty()||GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return false;const int count=AddFontResourceExW(path.c_str(),FR_PRIVATE|FR_NOT_ENUM,nullptr);if(count<=0)return false;file=path;selected=family;owned=true;return true;}
 void unload(){if(owned)RemoveFontResourceExW(file.c_str(),FR_PRIVATE|FR_NOT_ENUM,nullptr);owned=false;file.clear();selected=L"Arial";}
 const wchar_t* face()const{return selected.c_str();}bool loaded()const{return owned;}
};
}
