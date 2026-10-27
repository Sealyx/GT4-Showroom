#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include "Prize.hpp"
namespace gt4 {
inline constexpr float enthusiaFraming=1.32f;  
struct PreviewPose {float yaw=0,pitch=12,zoom=1,centerY=.43f;};
inline PreviewPose enthusiaPose(double seconds){
 struct Key{double t;PreviewPose p;bool whip;};
 static constexpr std::array<Key,8> keys={{{0,{2.18f,12,.90f,.43f},false},
 {1.46,{2.28f,12,.90f,.43f},false},{1.97,{5.40f,8,1.38f,.39f},true},
 {4.00,{5.58f,9,1.43f,.39f},false},{4.47,{10.05f,18,.99f,.42f},true},
 {6.47,{10.19f,19,1.05f,.42f},false},{6.97,{14.61f,14,1.0f,.43f},true},
 {8.50,{14.7463706f,12,.90f,.43f},false}}};
 const double time=std::isfinite(seconds)?std::max(0.,seconds):0;
 const double loops=std::floor(time/8.5),t=time-loops*8.5;
 for(size_t i=1;i<keys.size();++i)if(t<=keys[i].t){const auto&a=keys[i-1];const auto&b=keys[i];float u=float((t-a.t)/(b.t-a.t));if(b.whip)u=u*u*(3-2*u);return {a.p.yaw+(b.p.yaw-a.p.yaw)*u+float(loops*12.566370614359),a.p.pitch+(b.p.pitch-a.p.pitch)*u,a.p.zoom+(b.p.zoom-a.p.zoom)*u,a.p.centerY+(b.p.centerY-a.p.centerY)*u};}
 return keys.back().p;
}
enum class CloseAction {None,Exit,Spawn};
struct MenuFade {
 static constexpr double duration=.16,entryDuration=.22;
 CloseAction action=CloseAction::None;double began=0;bool blackDrawn=false;float initialBlack=0;
 void reset(){action=CloseAction::None;blackDrawn=false;began=0;initialBlack=0;}
 bool request(CloseAction next,double clock,float initial=0){if(action!=CloseAction::None)return false;action=next;began=clock;blackDrawn=false;initialBlack=std::clamp(initial,0.f,1.f);return true;}
 float opacity(double clock)const{if(action==CloseAction::None)return 0;float t=float(std::clamp((clock-began)/duration,0.,1.));t=smoothFade(t);return initialBlack+(1-initialBlack)*t;}
 void presented(double clock){if(action!=CloseAction::None&&clock-began>=duration-1e-9)blackDrawn=true;}
 bool ready(double clock)const{return action!=CloseAction::None&&blackDrawn&&clock-began>=duration-1e-9;}
};
}
