#pragma once
#include "Game.hpp"
namespace gt4 {
using FixedCameraNative=void(__thiscall*)(void*,const V3&,float,float,float);
struct OwnedFixedPose {V3 source,target,upOffset;float fov=50;};
inline bool ownedFixedInstance(void*self){
 void*camera=reinterpret_cast<void*>(0xB6F028);const auto active=game::field<uint8_t>(camera,0x59);
 return game::field<uint32_t>(camera,0xB0)==1&&game::field<uint8_t>(camera,0x2C)&&active<3&&self==static_cast<uint8_t*>(camera)+0x174+size_t(active)*0x238&&game::field<uint16_t>(self,0xC)==15;
}
inline bool processFixedShot(FixedCameraNative original,void*self,const V3&nativeTarget,float a,float b,float c,const OwnedFixedPose*owned){
 if(!owned||!ownedFixedInstance(self)){original(self,nativeTarget,a,b,c);return false;}
 // CCamera copies these vectors into CCam only when a mode transition begins.
 // Update the owned active CCam before native Process_Fixed consumes its copy.
 game::field<V3>(self,0x154)=owned->target;game::field<V3>(self,0x160)=owned->source;game::field<V3>(self,0x16C)=owned->upOffset;
 original(self,owned->target,a,b,c);game::field<float>(self,0xB4)=owned->fov;return true;
}
}
