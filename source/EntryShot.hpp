#pragma once
#include "Showroom.hpp"
namespace gt4 {
struct EntryCameraPose {V3 source,target,upOffset;float fov=50;};
inline float entrySmooth(float x){x=std::clamp(x,0.f,1.f);return x*x*x*(x*(x*6-15)+10);}
inline EntryCameraPose entryCameraPose(double age,V3 low,V3 high){
 const float t=float(std::clamp(age,0.,3.)),settle=entrySmooth((t-.55f)/2.45f),pan=entrySmooth(t/.95f);
 const float length=std::max(1.f,high.y-low.y),width=std::max(.65f,high.x-low.x),height=std::max(.5f,high.z-low.z),scale=std::max(length/4.7f,width/2.1f);
 const V3 center=(low+high)*.5f;const float distance=scale*(4.05f+1.15f*settle);
 EntryCameraPose p;p.source={center.x+distance*.74f,center.y+distance*.82f,low.z+height*(.48f+.25f*settle)};
 p.target={center.x-(1-pan)*5.8f*scale,center.y+length*.05f,low.z+height*.49f};
 const float envelope=entrySmooth(t/.30f),motionTime=float(std::max(0.,age));
 const float dx=envelope*(.025f*std::sin(3.1f*motionTime+.2f)+.01f*std::sin(7.7f*motionTime));
 const float dy=envelope*(.018f*std::sin(2.7f*motionTime-.6f)+.007f*std::sin(6.1f*motionTime+.7f));
 const float roll=envelope*(.0035f*std::sin(2.6f*motionTime)+.0011f*std::sin(5.4f*motionTime+.3f));
 const V3 direction=unit(p.target-p.source),right=unit(cross(direction,V3{0,0,1})),up=unit(cross(right,direction));
 p.source=p.source+right*(dx*scale*.35f)+up*(dy*scale*.3f);p.target=p.target+right*(dx*scale)+up*(dy*scale);
 p.upOffset=right*std::tan(roll);return p;
}

struct EntryPlayback{
 bool active=false;double age=0;
 void begin(){active=true;age=0;}
 bool step(double dt,bool paused,bool valid,bool seated,bool skipped,bool taskAlive){if(!active)return false;if(!valid||skipped){active=false;return false;}if(!paused)age+=std::clamp(dt,0.,.1);if(age>=12||(age>=3&&seated)||(age>.8&&!seated&&!taskAlive))active=false;return active;}
};
}
