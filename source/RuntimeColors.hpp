#pragma once
#include "RuntimeModels.hpp"
namespace gta {
// SA 1.0 US + FLA: both native instructions carry the live palette address.
// Never read the original .bss table after an adjuster relocates it.
inline uintptr_t vehiclePalette(){
 uint8_t a[3]{},b[3]{};uint32_t p=0,q=0;
 const uint8_t lea[]={0x8D,0x04,0x85},mov[]={0x8B,0x0C,0x85};
 if(!reader(0x447094,a,3)||!reader(0x6A6FF7,b,3)||
    std::memcmp(a,lea,3)||std::memcmp(b,mov,3)||
    !read(0x447097,p)||!read(0x6A6FFA,q)||!p||p!=q)return 0;
 return p;
}
inline bool vehicleColor(uint8_t id,uint32_t& rgb){
 const uintptr_t palette=vehiclePalette();if(!palette||(palette==0xB4E480&&id>=128))return false;
 uint32_t rgba=0;if(!read(palette+uintptr_t(id)*4,rgba))return false;
 rgb=((rgba&255)<<16)|(rgba&0xFF00)|((rgba>>16)&255);return true;
}
}
