#pragma once
#include "Game.hpp"
#include <unordered_set>
#include "VehFuncsBinding.hpp"
#include "WheelPose.hpp"
namespace gt4::nativePreview {
// VehFuncs assembles f_wheel variations in its vehicleRenderEvent.before,
// reached through CVehicle::Render -> CEntity::Render. Capture at the latter
// boundary and suppress only this private instance's actual world draw.
inline void* target=nullptr;inline bool failedPath=false;inline VehFuncsFrames visibility;
inline uint64_t prepared=0;inline int lastModel=-1,lastSources=0,lastRemaining=0,lastWheels=0;inline bool wheelMatrices=false;
inline int sourceGroups(void* frame,int depth=0){if(!frame||depth>80)return 0;int count=0,budget=0;while(frame&&budget++<4096){const char*name=reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(frame);if(name&&std::strncmp(name,"f_wheel",7)==0)++count;count+=sourceGroups(game::field<void*>(frame,0x98),depth+1);frame=game::field<void*>(frame,0x9C);}return count;}
inline bool frameVisible(void* frame){int depth=0;for(void*f=frame;f&&depth<96;f=game::field<void*>(f,4),++depth){if(!visibility.visible(f))return false;const char*name=reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(f);if(!previewHighDetail(name))return false;}return depth<96;}
inline void restWheelMatrices(void* vehicle){void*mi=gta::modelInfo(game::field<int16_t>(vehicle,0x22));if(!mi)return;const int kind=game::field<int>(mi,0x3C);wheelMatrices=prepareWheelPose(vehicle,kind,game::field<int16_t>(vehicle,0x22),[&](int node){return game::field<void*>(vehicle,0x648+size_t(node)*4)!=nullptr;},reinterpret_cast<WheelMatrixUpdate>(0x6AA290));}

inline std::unique_ptr<Mesh> result;
struct MaterialPin{void* material=nullptr;uint32_t color=0;std::array<uint8_t,12> surface{};};
inline std::vector<MaterialPin> retired;
// No native cleanup in a C++ static destructor: RenderWare may already be down.
// One last instance's pins can survive a reset until the next vehicle render.
inline void releaseRetired(){for(auto&p:retired)reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);retired.clear();}
struct Pins{std::vector<MaterialPin> values;std::unordered_set<void*> seen;bool valid=true;};
inline void* __cdecl pinAtomic(void* atomic,void* data){auto& pins=*static_cast<Pins*>(data);void* geometry=game::field<void*>(atomic,0x18);if(!geometry)return atomic;const int n=game::field<int>(geometry,0x24);auto** materials=game::field<void**>(geometry,0x20);if(n<0||n>1024||(!materials&&n)){pins.valid=false;return atomic;}for(int i=0;i<n;++i){void*m=materials[i];if(!m||!pins.seen.insert(m).second)continue;if(pins.values.size()>=4096){pins.valid=false;break;}auto& refs=game::field<int16_t>(m,0x18);if(refs<1||refs>=32766){pins.valid=false;break;}MaterialPin p;p.material=m;p.color=game::field<uint32_t>(m,4);std::memcpy(p.surface.data(),static_cast<uint8_t*>(m)+0xC,12);++refs;pins.values.push_back(p);}return atomic;}
inline void restore(const std::vector<MaterialPin>& pins){for(const auto&p:pins){game::field<uint32_t>(p.material,4)=p.color;std::memcpy(static_cast<uint8_t*>(p.material)+0xC,p.surface.data(),12);}}
inline bool onEntityRender(void* entity){
 if(!entity||(!target&&retired.empty()))return false;
 if(entity==target){
  // The preceding native vehicle event has flushed VehFuncs' last deferred
  // material resets. Keep pinned storage alive until this exact boundary.
  releaseRetired();result=std::make_unique<Mesh>();void*clump=game::field<void*>(entity,0x18);if(clump){
   // The factory alone never ran the wheel part of PreRender. Run only its
   // matrix helper, after VehFuncs has selected/copied the wheel children.
   // Avoid full PreRender: it emits lights, particles, shadows and audio.
   restWheelMatrices(entity);void*root=game::field<void*>(clump,4);if(root)reinterpret_cast<void*(__cdecl*)(void*)>(0x7F0910)(root);
   lastRemaining=sourceGroups(root);game::Extract extract{result.get(),0,frameVisible};reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,game::extractAtomic,&extract);++prepared;
  }return true;
 }
 if(!target&&(game::field<uint8_t>(entity,0x36)&7)==2)releaseRetired();
 return false;
}
inline bool enabledFor(int id){void*mi=gta::modelInfo(id);if(!game::vehicleInfo(mi))return false;const int kind=game::field<int>(mi,0x3C);return !failedPath&&GetModuleHandleW(L"VehFuncs.asi")&&(kind==0||kind==1||kind==2||kind==9||kind==10||kind==11);}
inline std::unique_ptr<Mesh> snapshot(int id,void(*prepare)(void*)=nullptr){
 if(!enabledFor(id)&&!prepare)return game::snapshot(id);
 struct Pool{void*objects;uint8_t*flags;int count;};auto*pool=game::at<Pool*>(0xB74494);bool free=false;if(pool&&pool->count>0&&pool->count<1000000)for(int i=0;i<pool->count;++i)if(pool->flags[i]&0x80){free=true;break;}if(!free)return prepare?nullptr:game::snapshot(id);
 // Factory only: do not add to CWorld, roads, traffic, mission cleanup or CJ.
 void* vehicle=reinterpret_cast<void*(__cdecl*)(int,uint8_t)>(0x421440)(id,2);if(!vehicle)return prepare?nullptr:game::snapshot(id);
 auto destroy=[&]{auto**vt=game::field<void**>(vehicle,0);reinterpret_cast<void*(__thiscall*)(void*,unsigned)>(vt[0])(vehicle,1);};
 void*clump=game::field<void*>(vehicle,0x18);if(!clump){destroy();return prepare?nullptr:game::snapshot(id);}
 if(prepare)prepare(vehicle);
 Pins pins;reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,pinAtomic,&pins);
 if(!pins.valid){for(auto&p:pins.values)reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);destroy();return prepare?nullptr:game::snapshot(id);}
 game::seedVehicle(vehicle,id);visibility.bind();lastModel=id;lastSources=sourceGroups(game::field<void*>(clump,4));lastRemaining=lastSources;lastWheels=0;
 if(game::field<int>(gta::modelInfo(id),0x3C)==0)for(int node:{2,5,4,7,3,6})if(game::field<void*>(vehicle,0x648+size_t(node)*4))++lastWheels;
 IDirect3DStateBlock9* saved=nullptr;if(auto*d=game::device())d->CreateStateBlock(D3DSBT_ALL,&saved);
 target=vehicle;result.reset();if(prepare)reinterpret_cast<void(__thiscall*)(void*)>(0x6D64F0)(vehicle);reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E60)(vehicle);if(prepare)reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E20)(vehicle);target=nullptr;
 if(saved){saved->Apply();saved->Release();}
 // Restore shared temporary colours/properties before returning to world draws.
 // Keep materials themselves referenced while VehFuncs' deferred reset pointers
 // may still name them, even after the private vehicle is destroyed.
 restore(pins.values);
 if(result){retired=std::move(pins.values);}else{failedPath=true;retired.insert(retired.end(),pins.values.begin(),pins.values.end());}
 destroy();
 if(result){result->compact();result->bounds();if(!result->empty())return std::move(result);result.reset();}
 return prepare?nullptr:game::snapshot(id);
}
}
