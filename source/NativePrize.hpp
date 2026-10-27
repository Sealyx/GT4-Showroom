#pragma once
#include "Game.hpp"
#include "PrizeStreaming.hpp"
#include "PrizeAbilities.hpp"
#include <set>
namespace gt4::prizeNative {
inline int extended16(const void* address){using Get=int(__cdecl*)(const void*);if(auto lib=GetModuleHandleW(L"$fastman92limitAdjuster.asi"))if(auto get=reinterpret_cast<Get>(GetProcAddress(lib,"GetExtendedIDfrom16bitBefore")))return get(address);int16_t value=-1;gta::reader(uintptr_t(address),&value,2);return value;}
inline bool validTxdSlot(int id){uintptr_t pool=0,flags=0;int count=0;uint8_t flag=0;return id>=0&&gta::read(0xC8800C,pool)&&pool&&gta::read(pool+4,flags)&&flags&&gta::read(pool+8,count)&&count>0&&count<1000000&&id<count&&gta::read(flags+size_t(id),flag)&&!(flag&0x80);}
template<class GetDictionary>bool txdReadyWith(int id,GetDictionary get){return id<0||(validTxdSlot(id)&&get(id)!=nullptr);}
inline bool txdReady(int id){return txdReadyWith(id,reinterpret_cast<void*(__cdecl*)(int)>(0x408340));}
inline bool upgradeReady(int id){return reinterpret_cast<bool(__cdecl*)(int)>(0x407820)(id);}
inline int pairFor(int id){if(id<0||id>32767)return -1;return reinterpret_cast<int16_t(__thiscall*)(void*,int16_t)>(0x4C74D0)(reinterpret_cast<void*>(0xB4E6D8),int16_t(id));}
inline std::vector<int> dependencies(const PrizePlan& plan){auto ids=prizeDependencies(plan);if(plan.kitResolved)for(int id:plan.upgrades){const int other=pairFor(id);if(other>=0&&std::find(ids.begin(),ids.end(),other)==ids.end())ids.push_back(other);}return ids;}
inline int paintTxd(const PrizePlan& plan){if(!plan.enablePaintjob)return -1;if(plan.paintResolved)return plan.remapTxd;void* mi=gta::modelInfo(plan.id);if(!game::vehicleInfo(mi))return -1;std::vector<int> remaps;for(int i=0;i<4;++i){const int id=extended16(static_cast<uint8_t*>(mi)+0x2FA+i*2);if(id>=0&&validTxdSlot(id))remaps.push_back(id);}return remaps.empty()?-1:remaps[size_t(plan.paintjob)%remaps.size()];}
inline std::vector<TuningOption> compatible(void* mi){
 std::vector<TuningOption> options;std::set<int> seen;
 auto add=[&](int id){void* part=gta::modelInfo(id);uint16_t flags=0;if(!part||!gta::read(uintptr_t(part)+0x12,flags))return;const int slot=tuningSlot(flags);if(slot<0||!seen.insert(id).second)return;options.push_back({id,slot,tuningEffect(slot)});};
 for(int i=0;i<18;++i)add(extended16(static_cast<uint8_t*>(mi)+0x2D6+i*2));
 const uint8_t wheelClass=game::field<uint8_t>(mi,0x4F);if(wheelClass>=1&&wheelClass<=4){const int count=reinterpret_cast<int(__cdecl*)(int)>(0x4C8740)(wheelClass-1);if(count>0&&count<=256)for(int i=0;i<count;++i)add(reinterpret_cast<int(__cdecl*)(int,int)>(0x4C8750)(wheelClass-1,i));}
 return options;
}
inline bool configure(PrizePlan& plan){if(!gta::modelReady(plan.id))return false;void* mi=gta::modelInfo(plan.id);if(!game::vehicleInfo(mi))return false;
 if(!plan.kitResolved){if(plan.modkit)plan.upgrades=chooseTuning(compatible(mi),plan.serial,plan.style);plan.kitResolved=true;}
 if(!plan.paintResolved){plan.remapTxd=-1;if(plan.enablePaintjob){int count=0;std::array<std::pair<int,int>,4> remaps{};for(int i=0;i<4;++i){const int id=extended16(static_cast<uint8_t*>(mi)+0x2FA+i*2);if(id>=0&&validTxdSlot(id))remaps[size_t(count++)]={i,id};}if(count){const auto choice=remaps[size_t(plan.paintjob%count)];plan.paintjob=choice.first;plan.remapTxd=choice.second;}}plan.paintResolved=true;}return true;
}
inline std::set<int> heldTxds,heldModels;
template<class AddRef,class Request>bool queueTxdWith(int id,AddRef addRef,Request request){if(!validTxdSlot(id))return false;if(heldTxds.insert(id).second)addRef(id);request(id,prizeStreamingFlags);return true;}
template<class RemoveRef>void releaseTxdsWith(RemoveRef removeRef){for(int id:heldTxds)if(validTxdSlot(id))removeRef(id);heldTxds.clear();}
inline void holdModel(int id){void* mi=gta::modelInfo(id);if(mi&&gta::modelReady(id)&&heldModels.insert(id).second)reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BA0)(mi);}
inline void release(){for(int id:heldModels)if(void* mi=gta::modelInfo(id))reinterpret_cast<void(__thiscall*)(void*)>(0x4C4BB0)(mi);heldModels.clear();releaseTxdsWith(reinterpret_cast<void(__cdecl*)(int)>(0x731A70));}
struct Backend {
 void queueModel(int id){game::queue(id);}bool modelReady(int id){return gta::modelReady(id);}
 bool validUpgrade(int id){return gta::modelInfo(id)!=nullptr;}
 void queueUpgrade(int id){reinterpret_cast<void(__cdecl*)(int,int)>(0x408C70)(id,prizeStreamingFlags);}bool upgradeReady(int id){return prizeNative::upgradeReady(id);}
 int remap(const PrizePlan& plan){return paintTxd(plan);}
 void queueTxd(int id){queueTxdWith(id,reinterpret_cast<void(__cdecl*)(int)>(0x731A00),reinterpret_cast<void(__cdecl*)(int,int)>(0x407100));}bool txdReady(int id){return prizeNative::txdReady(id);}
};
inline PrizeStreamReport report;inline PrizeLoadGate loadGate;
inline bool stream(PrizePlan& plan){configure(plan);Backend backend;report=probePrize(plan,backend);if(loadGate.shouldFlush(plan,report)){reinterpret_cast<void(__cdecl*)(bool)>(0x40EA10)(false);configure(plan);report=probePrize(plan,backend);}if(report.ready()){holdModel(plan.id);for(int id:dependencies(plan))holdModel(id);}return report.ready();}
inline std::set<void*> observed;
inline void* __cdecl observeAtomic(void* atomic,void*){if(void* info=reinterpret_cast<void*(__cdecl*)(void*)>(0x732260)(atomic))observed.insert(info);return atomic;}
inline int installedVisuals=0,expectedVisuals=0;
inline bool tune(void* vehicle,const PrizePlan& plan,bool preview=false){
 if(!vehicle||!plan||extended16(static_cast<uint8_t*>(vehicle)+0x22)!=plan.id)return false;
 if(!plan.modkit&&!plan.enablePaintjob){expectedVisuals=installedVisuals=0;return true;}
 const auto parts=prizeParts(plan);for(int id:dependencies(plan))if(!upgradeReady(id))return false;
 expectedVisuals=installedVisuals=0;
 for(int id:parts){uint16_t flags=0;void* mi=gta::modelInfo(id);if(!mi||!gta::read(uintptr_t(mi)+0x12,flags))continue;const bool effect=tuningEffect(tuningSlot(flags));if(!effect)++expectedVisuals;if(!preview||!effect)reinterpret_cast<void(__thiscall*)(void*,int)>(0x6E3290)(vehicle,id);}
 if(plan.enablePaintjob&&paintTxd(plan)>=0&&txdReady(paintTxd(plan))){game::preserveRemapColors(vehicle);reinterpret_cast<void(__thiscall*)(void*,int)>(0x6D0C00)(vehicle,plan.paintjob);}
 observed.clear();if(void* clump=game::field<void*>(vehicle,0x18))reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(clump,observeAtomic,nullptr);
 for(int id:parts){uint16_t flags=0;void* info=gta::modelInfo(id);if(info&&gta::read(uintptr_t(info)+0x12,flags)&&!tuningEffect(tuningSlot(flags))&&observed.count(info))++installedVisuals;}
 return !expectedVisuals||installedVisuals>0;
}
struct AbilityBackend {
 int reference(uintptr_t vehicle){return reinterpret_cast<int(__cdecl*)(void*)>(0x54FFC0)(reinterpret_cast<void*>(vehicle));}
 bool valid(const AbilityState& item){void* v=reinterpret_cast<void*(__cdecl*)(int)>(0x54FFF0)(item.reference);return uintptr_t(v)==item.pointer&&v&&extended16(static_cast<uint8_t*>(v)+0x22)==item.model;}
 void baseline(AbilityState& item){void* v=reinterpret_cast<void*>(item.pointer);item.mass=game::field<float>(v,0x8C);item.turnMass=game::field<float>(v,0x90);item.buoyancy=game::field<float>(v,0xA0);const uint32_t mask=item.ability==PrizeAbility::Proof?0x00840000u:1u;item.addedFlags=mask&~game::field<uint32_t>(v,0x40);}
 void enforce(AbilityState& item){void* v=reinterpret_cast<void*>(item.pointer);game::field<uint32_t>(v,0x40)|=item.ability==PrizeAbility::Proof?0x00840000u:1u;if(item.ability==PrizeAbility::HeavyRamming){game::field<float>(v,0x8C)=item.targetMass;game::field<float>(v,0x90)=item.targetTurnMass;game::field<float>(v,0xA0)=item.targetBuoyancy;}}
 void restore(const AbilityState& item){void* v=reinterpret_cast<void*>(item.pointer);game::field<uint32_t>(v,0x40)&=~item.addedFlags;if(item.ability==PrizeAbility::HeavyRamming){auto release=[&](size_t offset,float target,float baseline){float& value=game::field<float>(v,offset);if(value==target)value=baseline;};release(0x8C,item.targetMass,item.mass);release(0x90,item.targetTurnMass,item.turnMass);release(0xA0,item.targetBuoyancy,item.buoyancy);}}

};
inline AbilityBackend abilityBackend;inline PrizeAbilities<AbilityBackend> abilities;
inline bool ability(void* vehicle,const PrizePlan& plan){return abilities.add(uintptr_t(vehicle),plan.id,plan.ability,abilityBackend);}
inline const PrizePlan* preparing=nullptr;inline const Paint* preparingPaint=nullptr;inline bool prepared=false;
inline void preparePreview(void* vehicle){prepared=preparing&&tune(vehicle,*preparing,true);if(prepared&&preparingPaint){if(preparing->enablePaintjob&&paintTxd(*preparing)>=0)game::preserveRemapColors(vehicle);game::writePaint(vehicle,*preparingPaint);}}
}
