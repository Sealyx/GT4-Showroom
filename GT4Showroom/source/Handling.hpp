#pragma once
#include "RuntimeModels.hpp"
#include <cmath>
#include <cstdio>
#include <string>
namespace gt4 {
struct Handling {bool valid=false;char drive='?',engine='?';uint8_t gears=0;float mass=0,limit=0,acceleration=0;};
inline Handling handling(uint16_t id){Handling h;uint16_t multiply=0,lea=0;uint32_t stride=0,base=0;if(!gta::read(0x6B0B9C,multiply)||multiply!=0xC069||!gta::read(0x6B0BA2,lea)||lea!=0x808D||!gta::read(0x6B0B9E,stride)||!gta::read(0x6B0BA4,base)||stride<0xE0||stride>4096||!base)return h;const uintptr_t p=uintptr_t(base)+uintptr_t(id)*stride;uint32_t actual=0;float velocity=0,accel=0;if(!gta::read(p,actual)||actual!=id||!gta::read(p+4,h.mass)||!gta::read(p+0x74,h.drive)||!gta::read(p+0x75,h.engine)||!gta::read(p+0x76,h.gears)||!gta::read(p+0x7C,accel)||!gta::read(p+0x84,velocity))return {};if(!std::isfinite(h.mass)||h.mass<1||h.mass>1000000||!std::isfinite(velocity)||velocity<0||velocity>100||h.gears>10)return {};h.limit=velocity/(.277778f/50);h.acceleration=accel*2500*(h.drive=='4'?4:2);h.valid=true;return h;}
inline std::string stats(Handling h){if(!h.valid)return "Handling details unavailable";char b[160];std::snprintf(b,sizeof(b),"Type:%s    Mass:%.0fkg    Handling limit:%.0fkm/h    Gears:%u",h.drive=='4'?"4WD":h.drive=='F'?"FWD":h.drive=='R'?"RWD":"--",h.mass,h.limit,unsigned(h.gears));return b;}
}
