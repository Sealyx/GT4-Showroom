#include "../source/DiagnosticLog.hpp"
#include <filesystem>
#include <cassert>
int main(){
 wchar_t temp[MAX_PATH]{};GetTempPathW(MAX_PATH,temp);
 const auto root=std::filesystem::path(temp)/(L"GT4-log-test-"+std::to_wstring(GetCurrentProcessId()));std::filesystem::create_directories(root);
 const auto ini=(root/L"test.ini").wstring(),out=(root/L"test.log").wstring();DiagnosticLog log;
 log.write(out,"before config");assert(!std::filesystem::exists(out));
 log.reload(ini,L"GT4Showroom");log.write(out,"missing key");assert(!std::filesystem::exists(out));
 WritePrivateProfileStringW(L"GT4Showroom",L"EnableLog",L"1",ini.c_str());log.reload(ini,L"GT4Showroom");log.write(out,"enabled");assert(std::filesystem::file_size(out)>0);const auto size=std::filesystem::file_size(out);
 WritePrivateProfileStringW(L"GT4Showroom",L"EnableLog",L"0",ini.c_str());log.reload(ini,L"GT4Showroom");log.write(out,"disabled");assert(std::filesystem::file_size(out)==size);
 WritePrivateProfileStringW(L"GT4Showroom",L"EnableLog",nullptr,ini.c_str());log.reload(ini,L"GT4Showroom");log.write(out,"removed");assert(std::filesystem::file_size(out)==size);
 std::filesystem::remove(root/L"test.log");std::filesystem::remove(root/L"test.ini");std::filesystem::remove(root);puts("logging defaults / live toggles / preservation passed");
}
