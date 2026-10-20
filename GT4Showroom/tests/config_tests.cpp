#include "../source/FilterConfig.hpp"
#include "../source/HotkeyConfig.hpp"
#include "../source/StudioConfig.hpp"
#include "../source/Countries.hpp"
#include "../source/Showroom.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <cstdio>
int main(int argc,char**argv){assert(argc==2);const auto original=std::filesystem::absolute(argv[1]);auto c=gt4::readExclusions(original.wstring());assert(c.ids.size()==49&&c.ids.count(449)&&c.ids.count(590)&&c.invalid==0);
 const auto test=std::filesystem::temp_directory_path()/(L"GT4Showroom-config-"+std::to_wstring(GetCurrentProcessId())+L".ini");std::filesystem::copy_file(original,test);
 assert(WritePrivateProfileStringW(L"ExcludedModels",L"ExcludedModels",L"400,peren,Yosemite,20042,PEREN",test.c_str()));c=gt4::readExclusions(test.wstring());assert(c.ids.size()==2&&c.names.size()==2&&c.invalid==0);
 assert(WritePrivateProfileStringW(L"ExcludedModels",L"ExcludedModels",L"",test.c_str()));c=gt4::readExclusions(test.wstring());assert(c.ids.empty()&&c.names.empty());
 assert(WritePrivateProfileStringW(L"ExcludedModels",L"ExcludedModels",L"-1,1000000,400,999999999999999999999,@@@",test.c_str()));c=gt4::readExclusions(test.wstring());assert(c.ids.size()==1&&c.invalid==4);
 std::ofstream(test,std::ios::trunc)<<"[GT4Showroom]\nMusicVolume=25\n[ExcludedModels]\nPlanes=\n";WritePrivateProfileStringW(nullptr,nullptr,nullptr,test.c_str());c=gt4::readExclusions(test.wstring());assert(c.ids.size()==36&&!c.ids.count(539)&&c.ids.count(449));
 assert(WritePrivateProfileStringW(L"ExcludedModels",L"Trains",L"",test.c_str()));c=gt4::readExclusions(test.wstring());assert(c.ids.size()==30&&!c.ids.count(449));
 auto key=gt4::readHotkey(original.wstring());assert(key.vk==VK_F1&&!key.ctrl&&!key.invalid);
 assert(WritePrivateProfileStringW(L"GT4Showroom",L"OpenKey",L"G",test.c_str()));assert(WritePrivateProfileStringW(L"GT4Showroom",L"RequireCtrl",L"1",test.c_str()));key=gt4::readHotkey(test.wstring());assert(key.vk=='G'&&!key.ctrl);
 assert(WritePrivateProfileStringW(L"GT4Showroom",L"OpenKey",L"not a key",test.c_str()));key=gt4::readHotkey(test.wstring());assert(key.vk==VK_F1&&key.invalid&&!key.ctrl);
 assert(gt4::parseOpenKey(L" F12 ")==VK_F12&&gt4::parseOpenKey(L"F24")==VK_F24&&gt4::parseOpenKey(L"112")==VK_F1&&gt4::parseOpenKey(L"0x70")==VK_F1&&gt4::parseOpenKey(L"INSERT")==VK_INSERT);
 assert(gt4::parseOpenKey(L"0")=='0');assert(!gt4::parseOpenKey(L"F25")&&!gt4::parseOpenKey(L"256")&&!gt4::parseOpenKey(L"999999999999999999999999")&&!gt4::parseOpenKey(L"F2junk"));
 auto studio=gt4::readStudioConfig(original.wstring());assert(studio.sort.by==gt4::SortBy::Name&&!studio.sort.descending&&!studio.enterVehicle);
 assert(WritePrivateProfileStringW(L"GT4Showroom",L"SortBy",L"class",test.c_str()));assert(WritePrivateProfileStringW(L"GT4Showroom",L"SortOrder",L"desc",test.c_str()));assert(WritePrivateProfileStringW(L"GT4Showroom",L"SpawnInside",L"1",test.c_str()));studio=gt4::readStudioConfig(test.wstring());assert(studio.sort.by==gt4::SortBy::Name&&!studio.sort.descending&&studio.enterVehicle&&!studio.invalidSort); // obsolete sorting keys have no effect
 assert(WritePrivateProfileStringW(L"Countries",L"Unknown",L"20042,411,comet",test.c_str()));assert(WritePrivateProfileStringW(L"Countries",L"USA",L"411,comet,20042",test.c_str()));assert(WritePrivateProfileStringW(L"Countries",L"Japan",L"lucent",test.c_str()));
 const auto origins=gt4::readCountries(test.wstring());gt4::Entry origin;origin.id=411;origin.modelKey=gt4::modelNameKey("infernus");assert(origins.resolve(origin)==gt4::Country::Japan);origin.id=480;origin.modelKey=gt4::modelNameKey("comet");assert(origins.resolve(origin)==gt4::Country::Germany);origin.id=20042;origin.modelKey=gt4::modelNameKey("lucent");assert(origins.resolve(origin)==gt4::Country::USA);origin.id=20043;assert(origins.resolve(origin)==gt4::Country::Japan);origin.modelKey=gt4::modelNameKey("unassigned");assert(origins.resolve(origin)==gt4::Country::Unknown&&origins.overrides[0].ids.empty()&&origins.overrides[0].names.empty());
 std::filesystem::remove(test);std::puts("Win32 INI: exclusions, defaults, key/sort/spawn settings, add-on country IDs/names, ignored Unknown and immutable vanilla origins passed.");}


