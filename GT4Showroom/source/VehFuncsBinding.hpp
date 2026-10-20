#pragma once
#include "RuntimeModels.hpp"
#include <array>
namespace gt4 {
// Validate the public export's implementation before reading its frame bit.
// The attached 2.5.2 binary and upstream FramePlugin layout agree on +9/bit0.
// No fixed DLL base, private entry point or guessed plugin-allocation offset.
inline bool hideExportLayout(const uint8_t* b,size_t n){constexpr uint8_t pattern[]={0x55,0x8B,0xEC,0x8B,0x4D,0x08,0x03,0x0D,0,0,0,0,0x8A,0x41,0x09,0x24,0xFE,0x0A,0x45,0x0C,0x88,0x41,0x09,0x5D,0xC3};if(n<sizeof(pattern))return false;for(size_t i=0;i<sizeof(pattern);++i)if((i<8||i>11)&&b[i]!=pattern[i])return false;return true;}
struct VehFuncsFrames{
 int offset=-1;bool verified=false;
 void bind(){verified=false;offset=-1;const auto module=GetModuleHandleW(L"VehFuncs.asi");if(!module)return;const auto symbol=GetProcAddress(module,"Ext_ToggleFrameHide");std::array<uint8_t,25> bytes{};if(!symbol||!gta::processRead(uintptr_t(symbol),bytes.data(),bytes.size())||!hideExportLayout(bytes.data(),bytes.size()))return;uint32_t slot=0;std::memcpy(&slot,bytes.data()+8,4);int dynamic=-1;if(!gta::read(slot,dynamic))return;const int registered=reinterpret_cast<int(__cdecl*)(uint32_t)>(0x7F1290)(0x56454846);if(registered<0xA4||registered>8192||registered!=dynamic)return;offset=registered;verified=true;}
 bool visible(void* frame)const{if(!verified||!frame)return true;uint8_t flags=0;return !gta::read(uintptr_t(frame)+size_t(offset)+9,flags)||(flags&1)==0;}
};
}
