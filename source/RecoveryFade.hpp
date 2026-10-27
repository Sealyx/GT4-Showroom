#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace gt4 {
struct RecoveryFade {
 enum class Phase {Idle,Out,Black,Recover,In};Phase phase=Phase::Idle;double elapsed=0;uint64_t resumeWorld=0;
 static constexpr double outDuration=.22,blackDuration=.6,inDuration=.45;
 void reset(){phase=Phase::Idle;elapsed=0;resumeWorld=0;}
 void begin(){phase=Phase::Out;elapsed=0;resumeWorld=0;}
 bool active()const{return phase!=Phase::Idle;}
 bool waiting()const{return phase==Phase::Out||phase==Phase::Black;}
 bool readyToResume()const{return phase==Phase::Recover;}
 void markNativeResume(uint64_t world){resumeWorld=world;}
 void step(double dt,bool suspended,bool sceneReady){
  if(!active()||suspended||!std::isfinite(dt))return;dt=std::clamp(dt,0.,.1);
  if(phase==Phase::Recover){if(sceneReady){phase=Phase::In;elapsed=0;}return;}
  elapsed+=dt;
  if(phase==Phase::Out&&elapsed+1e-9>=outDuration){phase=Phase::Black;elapsed=0;}
  else if(phase==Phase::Black&&elapsed+1e-9>=blackDuration){phase=Phase::Recover;elapsed=0;}
  else if(phase==Phase::In&&elapsed+1e-9>=inDuration)reset();
 }
 float opacity()const{auto smooth=[](double t){t=std::clamp(t,0.,1.);return float(t*t*(3.-2.*t));};switch(phase){case Phase::Idle:return 0;case Phase::Out:return smooth(elapsed/outDuration);case Phase::In:return 1-smooth(elapsed/inDuration);default:return 1;}}
};
}
