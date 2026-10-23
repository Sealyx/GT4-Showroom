#pragma once
#include "LicenseFail.hpp"
namespace gt4 {
// Observe only. Native GameLogic owns all player, hospital, police and fade state.
struct OutcomeState {
 LicenseFailState view;uint32_t event=0;bool music=false;
 void reset(){view.reset();event=0;music=false;}
 bool step(uint8_t nativeState,uint32_t stamp,bool enabled,bool suspended,double dt){
  if(!enabled||(nativeState!=1&&nativeState!=2)){reset();return false;}
  const auto kind=nativeState==1?FailureKind::Wasted:FailureKind::Busted;
  if(!view.active||view.kind!=kind||event!=stamp){view.begin(1,kind);event=stamp;music=false;}
  if(!suspended)view.update(dt,{},1,1);
  return true;
 }
};
}
