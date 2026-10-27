#pragma once
#include "SecretFeaturesState.hpp"
namespace gt4 {
struct SecretFeaturesRecoveryState {
 SecretFeaturesState view;uint32_t event=0;bool music=false,resuming=false,drawn=false,quitPosted=false;
 uint64_t heartbeat=0,presented=0,began=0;SecretFeaturesChoice pending=SecretFeaturesChoice::None;double closing=0;
 void reset(){view.reset();event=0;music=resuming=drawn=quitPosted=false;heartbeat=presented=began=0;pending=SecretFeaturesChoice::None;closing=0;}
 bool matches(uint8_t nativeState,uint32_t stamp)const{return (nativeState==1||nativeState==2)&&event==stamp&&view.kind==(nativeState==1?FailureKind::Wasted:FailureKind::Busted);}
 bool observe(uint8_t nativeState,uint32_t stamp,bool enabled,uint64_t now){
  if(!enabled||(nativeState!=1&&nativeState!=2)){reset();return false;}
  const auto kind=nativeState==1?FailureKind::Wasted:FailureKind::Busted;
  if((!view.active&&!resuming)||!matches(nativeState,stamp)){reset();view.begin(1,kind);event=stamp;began=now;}
  heartbeat=now;return view.active;
 }
 bool hold(uint64_t now,uint8_t nativeState,uint32_t stamp,bool enabled,bool suspended)const{
  const bool startup=!drawn&&now>=began&&now-began<=1200&&!suspended;
  const bool live=drawn&&(suspended||(now>=presented&&now-presented<=500));
  return enabled&&view.active&&!resuming&&matches(nativeState,stamp)&&now>=heartbeat&&now-heartbeat<=350&&(startup||live);
 }
 void present(uint64_t now,bool ok){drawn=ok;if(ok)presented=now;}
 void resume(){resuming=true;view.active=false;drawn=false;}
};
}
