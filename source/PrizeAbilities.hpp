#pragma once
#include "Prize.hpp"
#include <vector>
#include <memory>
namespace gt4 {
// The game/vehicle plugins retain ownership of m_pHandlingData. Never replace
// it with an ASI-owned allocation: VehFuncs reads an extended tail and deletes
// that pointer in its vehicle destructor, even after this menu has closed.
struct AbilityState {
 int reference=-1,model=-1;uintptr_t pointer=0;PrizeAbility ability=PrizeAbility::None;
 float mass=0,turnMass=0,buoyancy=0,targetMass=0,targetTurnMass=0,targetBuoyancy=0;uint32_t addedFlags=0;
};
template<class Backend>class PrizeAbilities {
 std::vector<std::unique_ptr<AbilityState>> active;
 void restore(AbilityState& item,Backend& backend){if(backend.valid(item))backend.restore(item);}
public:
 size_t size()const{return active.size();}
 bool add(uintptr_t vehicle,int model,PrizeAbility ability,Backend& backend){
  if(ability==PrizeAbility::None)return true;
  for(auto& item:active)if(item->pointer==vehicle&&backend.valid(*item))return false;
  auto item=std::make_unique<AbilityState>();item->pointer=vehicle;item->model=model;item->ability=ability;item->reference=backend.reference(vehicle);if(item->reference<0||!backend.valid(*item))return false;
  backend.baseline(*item);
  if(ability==PrizeAbility::HeavyRamming){
   if(!std::isfinite(item->mass)||item->mass<1||item->mass>1000000||!std::isfinite(item->turnMass)||item->turnMass<=0||item->turnMass>1e12f||!std::isfinite(item->buoyancy)||item->buoyancy<=0||item->buoyancy>1e12f)return false;
   item->targetMass=item->mass*6;item->targetTurnMass=item->turnMass*2.5f;item->targetBuoyancy=item->buoyancy*6;
  }
  if(active.size()>=32){restore(*active.front(),backend);active.erase(active.begin());}
  active.push_back(std::move(item));maintain(backend);return true;
 }
 void maintain(Backend& backend){for(auto it=active.begin();it!=active.end();){auto& item=**it;if(!backend.valid(item)){it=active.erase(it);continue;}backend.enforce(item);++it;}}
 void clear(Backend& backend){for(auto& item:active)restore(*item,backend);active.clear();}
};
}
