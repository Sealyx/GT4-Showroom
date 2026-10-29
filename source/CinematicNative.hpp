#pragma once
#include "Game.hpp"
#include "CinematicVehicle.hpp"
namespace gt4::cinematicNative {
struct DoorFields {float open,closed;uint16_t direction;uint8_t axis,state;float angle,previous,speed;};
static_assert(sizeof(DoorFields)==0x18,"classic CDoor layout");
inline bool automobileKind(int kind){return kind==0||kind==1||kind==2||kind==11;}
inline CinematicPoint point(V3 p){return {p.x,p.y,p.z};}
using FrameMatrix=game::Matrix*(__cdecl*)(void*);
#ifdef GTA_NATIVE_PREVIEW_TEST
inline FrameMatrix frameMatrix=nullptr;
#else
inline const FrameMatrix frameMatrix=reinterpret_cast<FrameMatrix>(0x7F0990);
#endif
inline bool matrix(void*frame,game::Matrix&out){
 if(!frame||!frameMatrix)return false;auto*current=frameMatrix(frame);
 return current&&gta::read(uintptr_t(current),out)&&cinematicFinite(point(out.pos))&&cinematicFinite(point(out.right))&&cinematicFinite(point(out.up))&&cinematicFinite(point(out.forward));
}
inline std::array<void*,cinematicComponentCount> capture(void*vehicle,int kind,CinematicCapabilities&caps){
 std::array<void*,cinematicComponentCount> frames{};caps={};if(!vehicle||!automobileKind(kind))return frames;
 void*handling=nullptr;uint8_t drive=0;if(gta::read(uintptr_t(vehicle)+0x384,handling)&&handling&&gta::read(uintptr_t(handling)+0x74,drive))caps.allWheelSteering=drive=='4';
 for(size_t i=0;i<frames.size();++i){void*frame=nullptr;game::Matrix ltm{};if(!gta::read(uintptr_t(vehicle)+0x648+size_t(cinematicNativeNodes[i])*4,frame)||!matrix(frame,ltm))continue;
  auto&p=caps.parts[i];p.pivot=point(ltm.pos);p.axis={0,0,1};
  if(i<6){DoorFields door{};if(!gta::read(uintptr_t(vehicle)+0x5B8+i*sizeof(DoorFields),door)||door.axis>2||!std::isfinite(door.open)||!std::isfinite(door.angle)||std::abs(door.open-door.angle)>.01f+3.141593f||std::abs(door.open-door.angle)<.01f)continue;
   void*parent=nullptr;game::Matrix basis{};if(!gta::read(uintptr_t(frame)+4,parent)||!matrix(parent,basis))continue;
   p.axis=cinematicUnit(point(door.axis==0?basis.right:door.axis==1?basis.up:basis.forward));p.openAngle=door.open-door.angle;
  }
  p.present=true;frames[i]=frame;
 }
 return frames;
}
inline int component(void*frame,const std::array<void*,cinematicComponentCount>*frames){
 if(!frames)return -1;for(int depth=0;frame&&depth<96;++depth){for(size_t i=0;i<frames->size();++i)if((*frames)[i]&&(*frames)[i]==frame)return int(i);void*parent=nullptr;if(!gta::read(uintptr_t(frame)+4,parent))break;frame=parent;}return -1;
}
inline CinematicLamp namedLamp(void*frame){for(int depth=0;frame&&depth<96;++depth){const char*name=reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(frame);if(name){const size_t length=strnlen(name,128);if(length<128){const auto lamp=cinematicNamedLamp(std::string_view(name,length));if(lamp!=CinematicLamp::None)return lamp;}}void*parent=nullptr;if(!gta::read(uintptr_t(frame)+4,parent))break;frame=parent;}return CinematicLamp::None;}
}
