#pragma once
#include "Showroom.hpp"
#include "EntryMotion.hpp"
namespace gt4 {
struct EntryCameraPose {V3 source,target,upOffset;float fov=50;};
inline float entrySmooth(float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);}
inline EntryCameraPose entryCameraPose(double age,V3 low,V3 high){
 const float t=float(std::clamp(age,0.,3.)),settle=entrySmooth((t-.55f)/2.45f),pan=entrySmooth(t/.82f);
 const float length=std::max(1.f,high.y-low.y),width=std::max(.65f,high.x-low.x),height=std::max(.5f,high.z-low.z),scale=std::max(length/4.7f,width/2.1f);
 const V3 center=(low+high)*.5f;const float distance=scale*(3.95f+.95f*settle);
 EntryCameraPose p;p.source={center.x+distance*.73f,center.y+distance*.82f,low.z+height*(.55f+.19f*settle)};
 p.target={center.x-(1-pan)*5.8f*scale,center.y+length*.08f,low.z+height*.49f};
 const float frame=std::clamp(t*60.f,0.f,179.f);const size_t a=size_t(frame),b=std::min(a+1,entryMotion.size()-1);const float mix=frame-float(a);std::array<float,3> motion{};for(int i=0;i<3;++i)motion[size_t(i)]=entryMotion[a][size_t(i)]*(1-mix)+entryMotion[b][size_t(i)]*mix;
 // Convert measured image-plane drift into a small aim displacement at the
 // car's depth. Roll uses the fixed camera's native up-vector offset.
 const V3 direction=unit(p.target-p.source),right=unit(cross(direction,V3{0,0,1})),up=unit(cross(right,direction));
 const float plane=distance*2*std::tan(p.fov*pi/360.f);p.target=p.target+right*(-motion[0]*plane)+up*(motion[1]*plane*.5625f);
 p.upOffset=right*std::tan(-motion[2]);return p;
}
struct EntryPlayback{
 bool active=false;double age=0;
 void begin(){active=true;age=0;}
 bool step(double dt,bool paused,bool valid,bool seated,bool skipped,bool taskAlive){if(!active)return false;if(!valid||skipped){active=false;return false;}if(!paused)age+=std::clamp(dt,0.,.1);if(age>=12||(age>=3&&seated)||(age>.8&&!seated&&!taskAlive))active=false;return active;}
};
}
