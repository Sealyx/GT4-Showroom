#pragma once
#include "Showroom.hpp"
namespace gt4 {
struct FooterLayout{float y,fontHeight,countY,countHeight;FooterLayout(float h):y(h*.925f),fontHeight(18*h/720.f),countY(h*.929f),countHeight(15*h/720.f){}};
inline float sideOnHeading(V3 player,V3 vehicle){return std::atan2(-(vehicle.x-player.x),vehicle.y-player.y)+pi*.5f;}
inline float sideSpawnDistance(float lowX,float highX){const float width=std::max(std::abs(lowX),std::abs(highX));return std::isfinite(width)&&width>.1f&&width<30?std::max(2.f,width+1.f):2.6f;}
 
 
 
inline bool ivfLightMarker(uint32_t rgb){const auto r=(rgb>>16)&255,g=(rgb>>8)&255,b=rgb&255;
 return (r==255&&b==0&&((g>=173&&g<=175)||(g>=56&&g<=60)))||
        (g==255&&b==0&&r>=181&&r<=185)||(g==255&&r==0&&b>=198&&b<=200)||
        (b==255&&r==0&&g>=16&&g<=18);
}
inline uint32_t previewMaterialColor(uint32_t rgb){return ivfLightMarker(rgb)||rgb==0x01FFFF?0xFFFFFF:rgb;}
}
