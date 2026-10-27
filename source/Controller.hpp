#pragma once
#include <windows.h>
#include <cmath>
#include <algorithm>
#include "InputState.hpp"
namespace gt4 {
struct PadActions{
 bool pause=false;bool connected=false,accept=false,back=false,add=false,favorites=false,up=false,down=false,left=false,right=false,view=false,search=false;
 float orbitX=0,orbitY=0,zoom=0;
 bool previousHotspot=false,nextHotspot=false,zoomFromShoulders=false;
};
inline float deadAxis(int v,int limit,int zone){if(std::abs(v)<=zone)return 0;return std::copysign(std::min(1.f,float(std::abs(v)-zone)/float(limit-zone)),float(v));}
inline PadActions nativeController(const ControllerInput&p){PadActions o;auto b=[&](size_t off){return p.down(off);};auto a=[&](int i){return deadAxis(p.values[size_t(i)],128,28);};o.pause=b(0x18);o.accept=b(0x20);o.back=b(0x22);o.add=b(0x1C);o.favorites=b(0x1E);o.up=b(0x10)||a(1)<-.6f;o.down=b(0x12)||a(1)>.6f;o.left=b(0x14)||a(0)<-.6f;o.right=b(0x16)||a(0)>.6f;o.view=b(0x1A);o.search=b(0x24);o.orbitX=a(2);o.orbitY=a(3);o.previousHotspot=b(0x8);o.nextHotspot=b(0xC);o.zoom=(b(0xE)?1.f:0.f)-(b(0xA)?1.f:0.f);if(!o.zoom){o.zoom=(b(0xC)?1.f:0.f)-(b(0x8)?1.f:0.f);o.zoomFromShoulders=o.zoom!=0;}o.connected=o.pause||o.accept||o.back||o.add||o.favorites||o.up||o.down||o.left||o.right||o.view||o.search||o.orbitX||o.orbitY||o.zoom||o.previousHotspot||o.nextHotspot;return o;}
inline bool studioOpenInput(bool keyboard,bool open,const PadActions&pad){return keyboard||(!open&&pad.view);}
class Controller{
public:
 PadActions poll(const ControllerInput&native,uint64_t /*tick*/)const{return nativeController(native);}
};
}
