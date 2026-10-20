#pragma once
#include "Showroom.hpp"
namespace gt4 {
inline bool compactThumbnail(const Entry&e){
 if(e.kind==9||e.kind==10)return true; // native motorcycle / bicycle kinds
 for(int id:{448,461,462,463,468,471,521,522,523,581,586,481,509,510,571,530,539,574,583,485,572})if(e.id==id)return true;
 for(const char*name:{"kart","forklift","vortex","sweeper","tug","baggage","mower"})if(e.modelKey==modelNameKey(name))return true;
 return false;
}
inline Camera fittedThumbnail(float w,float h,V3 low,V3 high,float angle,float occupancy){
 const float span=std::max({high.x-low.x,high.y-low.y,high.z-low.z,1.f});
 Camera c(w,h,span,.5f);const V3 target{0,0,(high.z-low.z)*.45f};
 constexpr float elevation=.21f;c.eye=target+V3{0,-std::cos(elevation)*span*2.8f,std::sin(elevation)*span*2.8f};
 c.forward=unit(target-c.eye);c.right=unit(cross(c.forward,{0,0,1}));c.up=cross(c.right,c.forward);
 const float co=std::cos(angle),si=std::sin(angle);float xmin=1e9f,xmax=-1e9f,ymin=1e9f,ymax=-1e9f;
 for(int i=0;i<8;++i){V3 p{((i&1)?high.x:low.x)-(low.x+high.x)*.5f,((i&2)?high.y:low.y)-(low.y+high.y)*.5f,((i&4)?high.z:low.z)-low.z};p={p.x*co-p.y*si,p.x*si+p.y*co,p.z};float x,y,z,rhw;if(c.project(p,x,y,z,rhw)){xmin=std::min(xmin,x);xmax=std::max(xmax,x);ymin=std::min(ymin,y);ymax=std::max(ymax,y);}}
 const float fit=std::min(std::max(1.f,w-4)/std::max(.01f,xmax-xmin),std::max(1.f,h-4)/std::max(.01f,ymax-ymin))*std::clamp(occupancy,.1f,1.f);
 c.focal*=fit;c.offsetX=(w*.5f-(xmin+xmax)*.5f)*fit;c.cy=h*.5f+(h*.5f-(ymin+ymax)*.5f)*fit;return c;
}
}
