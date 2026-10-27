#pragma once
#include "Game.hpp"
#include <unordered_set>
#include "VehFuncsBinding.hpp"
#include "WheelPose.hpp"
#include "PreviewMaterialCycle.hpp"
#include "SpecialComponentPose.hpp"
#include "CinematicNative.hpp"
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
inline void(*observeVehicle)(void*)=nullptr;
inline bool copyGeometry=true,reached=false;
inline std::unordered_map<void*,int> markerSlots;
inline std::unordered_map<void*,CinematicLamp> lampSlots;
struct MaterialPin{void* material=nullptr;uint32_t color=0;void* texture=nullptr;std::array<uint8_t,12> surface{};};
inline std::vector<MaterialPin> retired;
// No native cleanup in a C++ static destructor: RenderWare may already be down.
// One last instance's pins can survive a reset until the next vehicle render.
inline void releaseRetired(){for(auto&p:retired){reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);if(p.texture)reinterpret_cast<bool(__cdecl*)(void*)>(0x7F3820)(p.texture);}retired.clear();}
struct Pins{std::vector<MaterialPin> values;std::unordered_set<void*> seen;bool valid=true;};
inline void* __cdecl pinAtomic(void* atomic,void* data){auto& pins=*static_cast<Pins*>(data);void* geometry=game::field<void*>(atomic,0x18);if(!geometry)return atomic;const int n=game::field<int>(geometry,0x24);auto** materials=game::field<void**>(geometry,0x20);if(n<0||n>1024||(!materials&&n)){pins.valid=false;return atomic;}for(int i=0;i<n;++i){void*m=materials[i];if(!m||!pins.seen.insert(m).second)continue;if(pins.values.size()>=4096){pins.valid=false;break;}auto& refs=game::field<int16_t>(m,0x18);if(refs<1||refs>=32766){pins.valid=false;break;}MaterialPin p;p.material=m;p.color=game::field<uint32_t>(m,4);p.texture=game::field<void*>(m,0);if(p.texture){auto&tr=game::field<int32_t>(p.texture,0x54);if(tr<1||tr>=0x3FFFFFFF){pins.valid=false;break;}++tr;}const uint32_t rgb=((p.color&255)<<16)|(p.color&0xFF00)|((p.color>>16)&255);constexpr uint32_t markers[]={0x3CFF00,0xFF00AF,0x00FFFF,0xFF00FF};for(int k=0;k<4;++k)if(rgb==markers[k])markerSlots[m]=k;std::memcpy(p.surface.data(),static_cast<uint8_t*>(m)+0xC,12);++refs;pins.values.push_back(p);}return atomic;}
inline void restore(const std::vector<MaterialPin>& pins){for(const auto&p:pins){game::field<uint32_t>(p.material,4)=p.color;std::memcpy(static_cast<uint8_t*>(p.material)+0xC,p.surface.data(),12);}}
inline bool onEntityRender(void* entity){
 if(!entity||(!target&&retired.empty()))return false;
 if(entity==target){
  // The preceding native vehicle event has flushed VehFuncs' last deferred
  // material resets. Keep pinned storage alive until this exact boundary.
  releaseRetired();reached=true;if(observeVehicle)observeVehicle(entity);if(!copyGeometry)return true;result=std::make_unique<Mesh>();void*clump=game::field<void*>(entity,0x18);if(clump){
   // The factory alone never ran the wheel part of PreRender. Run only its
   // matrix helper, after VehFuncs has selected/copied the wheel children.
   // Avoid full PreRender: it emits lights, particles, shadows and audio.
   restWheelMatrices(entity);prepareCementPose(entity,game::field<int16_t>(entity,0x22),[&](int node){return game::field<void*>(entity,0x648+size_t(node)*4);},reinterpret_cast<void(__thiscall*)(void*,void*,int,float,bool)>(0x6DBA30));void*root=game::field<void*>(clump,4);if(root)reinterpret_cast<void*(__cdecl*)(void*)>(0x7F0910)(root);
    lastRemaining=sourceGroups(root);void*mi=gta::modelInfo(game::field<int16_t>(entity,0x22));const auto frames=cinematicNative::capture(entity,mi?game::field<int>(mi,0x3C):-1,result->cinematic);game::Extract extract{result.get(),0,frameVisible,&markerSlots};extract.componentFrames=&frames;extract.component=cinematicNative::component;extract.namedLamp=cinematicNative::namedLamp;extract.lampSlots=&lampSlots;extract.plateTexture=game::field<void*>(entity,0x588);if(mi)extract.plateMaterial=game::field<void*>(mi,0x24);reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,game::extractAtomic,&extract);++prepared;
  }return true;
 }
 if(!target&&(game::field<uint8_t>(entity,0x36)&7)==2)releaseRetired();
 return false;
}
inline bool enabledFor(int id){void*mi=gta::modelInfo(id);if(!game::vehicleInfo(mi))return false;const int kind=game::field<int>(mi,0x3C);return !failedPath&&(kind>=0&&kind<=11);}
// Run one paired native material/render cycle on an existing instance. The
// observer runs after VehFuncs has resolved classes, pruned extras and copied
// wheel variants. It must never mutate shared model-template geometry.
template<class Device,class State>bool beginCaptureState(Device*device,State*&saved){saved=nullptr;return device&&SUCCEEDED(device->CreateStateBlock(D3DSBT_ALL,&saved))&&saved;}
inline std::unique_ptr<Mesh> captureVehicle(void* vehicle,int id,void(*prepare)(void*)=nullptr){
 if(!vehicle||target)return nullptr;void*clump=game::field<void*>(vehicle,0x18);if(!clump)return nullptr;
 IDirect3DStateBlock9* saved=nullptr;if(!beginCaptureState(game::device(),saved))return nullptr;
 if(prepare)prepare(vehicle);
 markerSlots.clear();lampSlots.clear();Pins pins;reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,pinAtomic,&pins);for(const auto&p:pins.values){const auto rgba=p.color;const auto lamp=cinematicMarkerLamp(((rgba&255)<<16)|(rgba&0xFF00)|((rgba>>16)&255));if(lamp!=CinematicLamp::None)lampSlots.emplace(p.material,lamp);}
  if(!pins.valid){for(auto&p:pins.values){reinterpret_cast<bool(__cdecl*)(void*)>(0x74DA20)(p.material);if(p.texture)reinterpret_cast<bool(__cdecl*)(void*)>(0x7F3820)(p.texture);}saved->Apply();saved->Release();return nullptr;}
 game::seedVehicle(vehicle,id);visibility.bind();lastModel=id;lastSources=sourceGroups(game::field<void*>(clump,4));lastRemaining=lastSources;lastWheels=0;
 if(copyGeometry&&game::field<int>(gta::modelInfo(id),0x3C)==0)for(int node:{2,5,4,7,3,6})if(game::field<void*>(vehicle,0x648+size_t(node)*4))++lastWheels;
 target=vehicle;result.reset();reached=false;
 previewMaterialCycle(vehicle,reinterpret_cast<void(__thiscall*)(void*)>(0x6D64F0),reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E60),reinterpret_cast<void(__thiscall*)(void*)>(0x6D0E20));target=nullptr;markerSlots.clear();lampSlots.clear();
 if(saved){saved->Apply();saved->Release();}
 restore(pins.values);
 if(reached)retired=std::move(pins.values);else{failedPath=true;retired.insert(retired.end(),pins.values.begin(),pins.values.end());}
 if(result){result->compact();result->bounds();if(!result->empty())return std::move(result);result.reset();}return nullptr;
}
inline std::unique_ptr<Mesh> snapshot(int id,void(*prepare)(void*)=nullptr){
 const bool plugin=GetModuleHandleW(L"VehFuncs.asi")||GetModuleHandleW(L"ImVehFt.asi")||GetModuleHandleW(L"IVF.asi");auto fallback=[&]()->std::unique_ptr<Mesh>{return plugin?nullptr:game::snapshot(id);};
 if(!enabledFor(id)&&!prepare)return copyGeometry?fallback():nullptr;
 struct Pool{void*objects;uint8_t*flags;int count;};auto*pool=game::at<Pool*>(0xB74494);bool free=false;if(pool&&pool->count>0&&pool->count<1000000)for(int i=0;i<pool->count;++i)if(pool->flags[i]&0x80){free=true;break;}if(!free)return prepare||!copyGeometry?nullptr:fallback();
 game::ComponentChoiceScope components(id);void*vehicle=reinterpret_cast<void*(__cdecl*)(int,uint8_t)>(0x421440)(id,2);if(!vehicle)return prepare||!copyGeometry?nullptr:fallback();
 auto mesh=captureVehicle(vehicle,id,prepare);
 auto**vt=game::field<void**>(vehicle,0);reinterpret_cast<void*(__thiscall*)(void*,unsigned)>(vt[0])(vehicle,1);
 return mesh?std::move(mesh):prepare||!copyGeometry?nullptr:fallback();
}
struct InspectionScope{bool saved=copyGeometry;InspectionScope(){copyGeometry=false;}~InspectionScope(){copyGeometry=saved;}};
inline bool inspect(int id){InspectionScope scope;reached=false;snapshot(id);return reached;}
inline bool initialize(void*vehicle,int id){InspectionScope scope;reached=false;captureVehicle(vehicle,id);return reached;}
}
