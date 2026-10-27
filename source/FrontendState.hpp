#pragma once
#include "RuntimeModels.hpp"
namespace gt4 {
inline constexpr uintptr_t frontendBase=0xBA6748;
inline constexpr uintptr_t menuActiveAddress=frontendBase+0x5C;
inline constexpr uintptr_t startLoadAddress=frontendBase+0x5D;
inline constexpr uintptr_t loadingDataAddress=frontendBase+0x60;
static_assert(menuActiveAddress==0xBA67A4&&startLoadAddress==0xBA67A5&&loadingDataAddress==0xBA67A8);
struct FrontendState {
 bool valid=false;uint8_t menuByte=1,startLoadByte=0,loadingByte=1;
 bool menuActive()const{return !valid||menuByte!=0;}
 bool loadingData()const{return !valid||loadingByte!=0;}
 bool startLoadRequested()const{return startLoadByte!=0;}
};
inline FrontendState readFrontendState(){
 FrontendState s;
 s.valid=gta::read(menuActiveAddress,s.menuByte)&&gta::read(startLoadAddress,s.startLoadByte)&&gta::read(loadingDataAddress,s.loadingByte);
 return s;
}
}
