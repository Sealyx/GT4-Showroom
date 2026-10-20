#include "../source/PrivateFont.hpp"
#include <cassert>
#include <cstdio>
#include <cwchar>
#include <filesystem>
struct Selected{std::wstring name;LONG weight=0;};
Selected selected(const wchar_t*face){HDC dc=CreateCompatibleDC(nullptr);HFONT f=CreateFontW(-29,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,ANSI_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,face);auto old=SelectObject(dc,f);wchar_t name[128]{};GetTextFaceW(dc,128,name);TEXTMETRICW metrics{};assert(GetTextMetricsW(dc,&metrics));SelectObject(dc,old);DeleteObject(f);DeleteDC(dc);return {name,metrics.tmWeight};}
int main(int argc,char**argv){assert(argc==3);const wchar_t*families[]={L"Michroma",L"Liberation Sans"};for(int i=0;i<2;++i){const auto before=selected(families[i]);gt4::PrivateFont font;assert(!font.load(L"Z:/missing-GT4-font.ttf",families[i]));assert(!font.loaded());assert(font.load(std::filesystem::path(argv[i+1]).wstring(),families[i]));const auto during=selected(families[i]);assert(_wcsicmp(during.name.c_str(),families[i])==0);if(i==1)assert(during.weight>=700);font.unload();assert(!font.loaded());assert(selected(families[i]).name==before.name);std::wprintf(L"Private font passed: %ls, selected weight %ld; resource removed after use.\n",during.name.c_str(),during.weight);}}
