#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <windows.h>
namespace gt4 {
struct AudioLevels {
 std::array<int,3> value{{100,45,80}};
 void normalize(){for(auto&v:value)v=std::clamp(v,0,100);}
 bool operator==(const AudioLevels&o)const{return value==o.value;}
 bool operator!=(const AudioLevels&o)const{return !(*this==o);}
 float music()const{return float(value[0]*value[1])/100.f;}
 float effects()const{return float(value[0]*value[2])/100.f;}
};
inline AudioLevels readAudioLevels(const std::filesystem::path&path){
 AudioLevels result;std::ifstream input(path,std::ios::binary);std::array<uint8_t,12> bytes{};
 if(!input.read(reinterpret_cast<char*>(bytes.data()),bytes.size())||input.peek()!=std::char_traits<char>::eof())return result;
 const std::array<uint8_t,5> magic{{'G','T','4','A',1}};if(!std::equal(magic.begin(),magic.end(),bytes.begin()))return result;
 uint8_t check=0xA7;for(size_t i=0;i<11;++i)check^=bytes[i];if(bytes[11]!=check)return result;
 for(size_t i=0;i<3;++i){if(bytes[i+5]>100)return AudioLevels{};result.value[i]=bytes[i+5];}return result;
}
inline bool saveAudioLevels(const std::filesystem::path&path,AudioLevels levels){
 levels.normalize();std::array<uint8_t,12> bytes{{'G','T','4','A',1,uint8_t(levels.value[0]),uint8_t(levels.value[1]),uint8_t(levels.value[2]),0,0,0,0}};
 bytes[11]=0xA7;for(size_t i=0;i<11;++i)bytes[11]^=bytes[i];
 std::error_code error;std::filesystem::create_directories(path.parent_path(),error);if(error)return false;
 auto temporary=path;temporary+=L".tmp";
 bool written=false;{std::ofstream output(temporary,std::ios::binary|std::ios::trunc);written=bool(output.write(reinterpret_cast<const char*>(bytes.data()),bytes.size())&&output.flush());}
 if(!written){DeleteFileW(temporary.c_str());return false;}
 if(!MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){DeleteFileW(temporary.c_str());return false;}return true;
}
}
