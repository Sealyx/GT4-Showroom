#pragma once
#include <windows.h>
#include "Catalogue.hpp"
namespace gt4 {
struct StudioConfig{SortSettings sort{SortBy::Name,false};bool enterVehicle=false,invalidSort=false;};
// User sorting belongs to the in-game context menu. Legacy INI keys are ignored.
inline StudioConfig readStudioConfig(const std::wstring&ini){StudioConfig c;c.enterVehicle=GetPrivateProfileIntW(L"GT4Showroom",L"SpawnInside",0,ini.c_str())!=0;return c;}
inline SortSettings menuSort(int mode){switch(mode){case 1:return {SortBy::Name,true};case 2:return {SortBy::Class,false};case 3:return {SortBy::TopSpeed,true};case 4:return {SortBy::Country,false};case 5:return {SortBy::Id,false};default:return {SortBy::Name,false};}}
}
