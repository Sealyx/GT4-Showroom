#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <memory>
#include "ModelNames.hpp"
#include "PreviewMotion.hpp"
#include "Countries.hpp"
#include "StudioUI.hpp"
namespace gt4 {
constexpr float pi=3.14159265358979323846f;
struct V3 { float x=0,y=0,z=0; };
inline V3 operator+(V3 a,V3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline V3 operator-(V3 a,V3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline V3 operator*(V3 a,float s){return {a.x*s,a.y*s,a.z*s};}
inline float dot(V3 a,V3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline V3 cross(V3 a,V3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline V3 unit(V3 a){const float n=std::sqrt(dot(a,a));return n>.00001f?a*(1.f/n):V3{0,0,1};}
inline float ease(float x){x=std::clamp(x,0.f,1.f);return x*x*(3-2*x);}
struct Paint {uint8_t ids[4]={0,0,0,0}; std::string name;uint32_t rgb[4]={0x94999C,0x94999C,0x94999C,0x94999C};};
inline const char* classLabel(int value){static const char* labels[]={"NORMAL","POOR","RICH","EXECUTIVE","WORKER","HEAVY","TAXI","MOPED","MOTORBIKE","LEISURE BOAT","WORK BOAT","BICYCLE"};return value>=0&&value<12?labels[value]:"SPECIAL";}
struct Entry {Country country=Country::Unknown;int id=0,kind=0,classId=0;uint32_t modelKey=0;float topSpeed=-1;std::string category="NORMAL",name,brand="SAN ANDREAS",title,year,power,torque,information; std::vector<Paint> paints; bool favorite=false;};
struct Livery {int index=-1,txd=-1;std::shared_ptr<void> texture;};
enum class Page {Cars,Paint,Livery,Prize};
enum class Cue {None,Move,Accept,Back,Favorite,Open,Spawn,Error};
struct Model {
 StudioUI ui;std::vector<Livery> liveries;size_t livery=0;int liveriesModel=-1;
 float scrollTarget()const{return float(std::clamp(int(selected)-5,0,std::max(0,int(count)-11)));}
 SecretUnlocksConfig secretUnlocks;MenuFade fade;bool enthusia=false;double motionBegan=0;PrizePlan prize;double prizeBegan=0;size_t beforePrize=0;bool prizeReady=false;std::mt19937 random{5489u};
 void leavePrize(){if(page==Page::Prize){selected=beforePrize;page=Page::Cars;paint=0;changed=motionBegan=clock;scroll=scrollTarget();prize={};prizeReady=false;status.clear();}}
 bool busy()const{return fade.action!=CloseAction::None||ui.transition.active();}
 void recovered(){fade.reset();spawn=false;opened=clock;}
 void presented(){fade.presented(clock);ui.transition.presented(clock);}
 bool open=false,spawn=false,dragging=false;float dragGrip=0; Page page=Page::Cars; size_t selected=0,paint=0;
 double clock=0,changed=0,opened=0,stripChanged=0; float scroll=0,paintMix=0; size_t count=0; float pointerX=.5f,pointerY=.79f; bool pointerVisible=true;
 Cue cue=Cue::None; std::string status;bool searching=false,searchFailed=false;std::string query,searchNotice;double searchUntil=0;
 void beginSearch(){if(busy())return;ui.menu.open=false;ui.cinematic.leave();leavePrize();searching=true;query.clear();searchNotice.clear();searchFailed=false;dragging=false;spawn=false;if(page==Page::Paint||page==Page::Livery){page=Page::Cars;stripChanged=clock;}}
 void submitSearch(const std::vector<Entry>&cars){if(busy())return;const auto command=lowerName(trimName(query));if(command=="realbig"){const auto next=rollPrize(cars,random,secretUnlocks);if(!next){searchFailed=true;searchNotice="REALBIG: NO MATCHING MODELS";searchUntil=clock+2.8;cue=Cue::Error;return;}if(page!=Page::Prize)beforePrize=selected;prize=next;for(size_t i=0;i<cars.size();++i)if(cars[i].id==prize.id){selected=i;break;}page=Page::Prize;paint=size_t(prize.paint);prizeBegan=clock;opened=clock;prizeReady=false;status.clear();searching=false;searchNotice.clear();dragging=false;cue=Cue::None;return;}leavePrize();if(command=="enthusia"||command=="gt4"){enthusia=command=="enthusia";motionBegan=clock;searching=false;searchFailed=false;searchNotice=enthusia?"ENTHUSIA PREVIEW":"GT4 PREVIEW";searchUntil=clock+2.8;cue=Cue::Accept;return;}const auto index=findModel(cars,query);searchFailed=index==cars.size();searchUntil=clock+2.8;if(searchFailed){searchNotice="MODEL NOT FOUND";cue=Cue::Error;}else{select(index,cars);searching=false;searchNotice="MODEL FOUND: "+cars[index].name+"  /  "+std::to_string(cars[index].id);cue=Cue::Accept;}}
 void begin(size_t total){if(!total)return;count=total;ui.resetTransient();ui.pointerMoved=clock;livery=0;liveries.clear();liveriesModel=-1;fade.reset();prize={};prizeReady=false;motionBegan=clock;open=true;searching=false;query.clear();searchNotice.clear();spawn=false;dragging=false;page=Page::Cars;paint=0;selected=std::min(selected,count-1);opened=changed=clock;stripChanged=clock;scroll=scrollTarget();pointerX=.5f;pointerY=.79f;paintMix=0;status.clear();cue=Cue::Open;}
 void step(double seconds){seconds=std::isfinite(seconds)?std::clamp(seconds,0.,1.):0;clock+=seconds;ui.cinematic.step(seconds);ui.transition.step(clock);scroll+=(scrollTarget()-scroll)*float(1-std::exp(-seconds*14));const float target=(page==Page::Paint||page==Page::Livery)?1.f:0.f;paintMix+=(target-paintMix)*float(1-std::exp(-seconds*18));}
 void select(size_t index,const std::vector<Entry>& cars){if(!open||busy()||index>=cars.size()||index==selected)return;count=cars.size();selected=index;paint=livery=0;changed=motionBegan=clock;status.clear();cue=Cue::Move;}
 void exit(){if(!open||busy())return;spawn=false;dragging=false;searching=false;fade.request(CloseAction::Exit,clock,blackAlpha());cue=Cue::Back;}
 void pointerMove(float dx,float dy,float width,float height){if(!std::isfinite(dx)||!std::isfinite(dy)||width<=0||height<=0)return;ui.mouse(clock,dx,dy);pointerX=std::clamp(pointerX+dx/width,0.f,1.f);pointerY=std::clamp(pointerY+dy/height,0.f,1.f);}
 void move(int dir,const std::vector<Entry>& cars){if(!open||busy()||cars.empty()||!dir||page==Page::Prize)return;size_t* value=page==Page::Cars?&selected:page==Page::Paint?&paint:&livery;const size_t n=page==Page::Cars?cars.size():page==Page::Paint?std::max(size_t(1),cars[selected].paints.size()):std::max(size_t(1),liveries.size());const auto next=size_t(((int64_t(*value)+dir)%int64_t(n)+int64_t(n))%int64_t(n));if(next==*value)return;*value=next;ui.dock=true;if(page==Page::Cars){count=cars.size();changed=motionBegan=clock;paint=livery=0;status.clear();}cue=Cue::Move;}
 void accept(const std::vector<Entry>& cars){if(!open||busy()||cars.empty())return;if(page==Page::Prize){if(prizeReady&&clock-prizeBegan>=prizeReadyTime){spawn=true;fade.request(CloseAction::Spawn,clock,blackAlpha());cue=Cue::Spawn;}return;}if(page==Page::Cars){page=Page::Paint;stripChanged=clock;cue=Cue::Accept;}else if(page==Page::Paint&&liveries.size()>1){page=Page::Livery;stripChanged=clock;cue=Cue::Accept;}else{spawn=true;dragging=false;fade.request(CloseAction::Spawn,clock,blackAlpha());cue=Cue::Spawn;}}
 void back(){if(!open||busy())return;if(ui.menu.open){ui.menu.open=false;return;}if(ui.cinematic.active){ui.cinematic.leave();stripChanged=clock;cue=Cue::Back;return;}if(page==Page::Cars&&ui.favoritesOnly){ui.transition.request(StudioAction::AllCars,clock);cue=Cue::Back;return;}spawn=false;dragging=false;if(page==Page::Prize){leavePrize();stripChanged=clock;cue=Cue::Back;return;}if(page==Page::Livery){page=Page::Paint;stripChanged=clock;cue=Cue::Back;}else if(page==Page::Paint){page=Page::Cars;stripChanged=clock;cue=Cue::Back;}else exit();}
 void favorite(std::vector<Entry>& cars){if(open&&!busy()&&page!=Page::Prize&&!cars.empty()){cars[selected].favorite=!cars[selected].favorite;cue=Cue::Favorite;}}
 float carAlpha()const{if(enthusia)return ease(float((clock-changed)/.3));return ease(float((clock-changed-.34)/1.05));}
 float scale()const{if(enthusia)return 1.f;return 1.f+.24f*(1-carAlpha());}
 float heading()const{return float(clock-changed)*2*pi/20.f+.16f;}
 float titleMix()const{return ease(float(std::sin((clock-changed)*1.4)*1.8));}
 float opening()const{return smoothFade(float((clock-opened)/MenuFade::entryDuration));}
 float blackAlpha()const{return std::max(fade.action!=CloseAction::None?fade.opacity(clock):1-opening(),ui.transition.alpha(clock));}
};
struct Rect {
 float x=0,y=0,w=0,h=0;
 bool contains(float px,float py)const{return px>=x&&py>=y&&px<x+w&&py<y+h;}
};
struct Carousel {
 float width,height,scale,cell,left,y;int first,last;const Model& model;
 Carousel(float w,float h,const Model& s,size_t n):width(w),height(h),scale(h/720.f),cell(std::max(w/18.5f,std::min(100*scale,(w-140*scale)/11.5f))),left((w-11.38f*cell)*.5f),y(h*.749f+30*scale*s.paintMix),first(std::max(0,int(std::floor(s.scroll-left/cell))-2)),last(std::min(int(n),int(std::ceil(s.scroll+(w-left)/cell))+2)),model(s){
  while(first<last&&box(first).x+box(first).w<=0)++first;
  while(last>first&&box(last-1).x>=w)--last;
 }

 Rect box(int i)const{const bool selected=size_t(i)==model.selected;return {left+(i-model.scroll)*cell+(size_t(i)>model.selected?.38f*cell:0),y-(selected?15:0)*scale,(selected?132:94)*cell/100,(selected?61:43)*scale};}
 int hit(float x,float py)const{for(int i=first;i<last;++i)if(box(i).contains(x,py))return i;return -1;}
};
struct BrowseBar {
 Rect track,thumb,hit;float travel=0;
 BrowseBar(float w,float h,const Model& s,size_t count){const float scale=h/720.f;track={66*scale,h*.897f,std::max(0.f,w-132*scale),4*scale};thumb.w=std::min(track.w,std::max(40*scale,track.w*std::min(1.f,11.f/float(std::max(size_t(1),count)))));thumb.h=3*scale;travel=track.w-thumb.w;thumb.x=track.x+travel*(count>1?float(s.selected)/float(count-1):0);thumb.y=track.y;hit={track.x,track.y-10*scale,track.w,24*scale};}
 size_t index(float x,float grip,size_t count)const{return count>1&&travel>0?size_t(std::lround(std::clamp((x-track.x-grip)/travel,0.f,1.f)*float(count-1))):0;}
};
inline Rect paintBox(float w,float h,size_t i,size_t count){const float scale=h/720.f;return {w*.5f-float(count)*10*scale+i*20*scale,h*.753f,14*scale,48*scale};}
inline Rect liveryBox(float w,float h,size_t i,size_t count){const float s=h/720.f,cw=52*s;return {w*.5f-float(count)*cw*.5f+i*cw,h*.753f,46*s,48*s};}
inline void pointerClick(Model& model,const std::vector<Entry>& cars,float w,float h){
 if(!model.open||model.busy()||cars.empty())return;const float x=model.pointerX*w,y=model.pointerY*h;
 if(model.page==Page::Cars){const Carousel layout(w,h,model,cars.size());const int hit=layout.hit(x,y);if(hit>=0){if(size_t(hit)==model.selected)model.accept(cars);else model.select(size_t(hit),cars);}}
 else if(model.page==Page::Prize){if(Rect{(w-prizeButtonWidth(w,h))*.5f,h*prizeButtonTop,prizeButtonWidth(w,h),h*prizeButtonHeight}.contains(x,y))model.accept(cars);}
 else if(model.page==Page::Livery){for(size_t i=0;i<model.liveries.size();++i)if(liveryBox(w,h,i,model.liveries.size()).contains(x,y)){model.livery=i;model.cue=Cue::Move;return;}}
 else{const size_t n=std::max(size_t(1),cars[model.selected].paints.size());for(size_t i=0;i<n;++i)if(paintBox(w,h,i,n).contains(x,y)){model.paint=i;model.cue=Cue::Move;return;}}
}
inline void pointerButton(Model& model,const std::vector<Entry>& cars,float w,float h,bool down,bool pressed){
 if(!model.open||model.busy()||cars.empty())return;const float x=model.pointerX*w,y=model.pointerY*h;
 if(!down||model.page!=Page::Cars){model.dragging=false;}
 if(model.page==Page::Cars){const BrowseBar bar(w,h,model,cars.size());
  if(pressed&&bar.hit.contains(x,y)){model.dragging=true;model.dragGrip=(x>=bar.thumb.x&&x<=bar.thumb.x+bar.thumb.w)?x-bar.thumb.x:bar.thumb.w*.5f;}
  if(model.dragging){model.select(bar.index(x,model.dragGrip,cars.size()),cars);model.scroll=model.scrollTarget();return;}
 }
 if(pressed)pointerClick(model,cars,w,h);
}
inline void dockPointer(Model&s,const std::vector<Entry>&cars,float w,float h,double dt){
 if(!s.open||s.searching||s.ui.modal()||s.ui.cinematic.active||s.ui.cinematic.mix>.01f||s.dragging||cars.empty()||s.selected>=cars.size())return;
 if(!s.ui.dock)return;float x=.5f,y=.79f;
 if(s.page==Page::Cars){const auto b=Carousel(w,h,s,cars.size()).box(int(s.selected));x=(b.x+b.w*.68f)/w;y=(b.y+b.h*.68f)/h;}
 else if(s.page==Page::Paint){const auto b=paintBox(w,h,s.paint,std::max(size_t(1),cars[s.selected].paints.size()));x=(b.x+b.w*.65f)/w;y=(b.y+b.h*.62f)/h;}
 else if(s.page==Page::Livery){const auto b=liveryBox(w,h,s.livery,s.liveries.size());x=(b.x+b.w*.65f)/w;y=(b.y+b.h*.62f)/h;}else return;const float a=float(1-std::exp(-std::max(0.,dt)*14));s.pointerX+=(std::clamp(x,0.f,1.f)-s.pointerX)*a;s.pointerY+=(std::clamp(y,0.f,1.f)-s.pointerY)*a;if(std::abs((s.pointerX-x)*w)+std::abs((s.pointerY-y)*h)<.3f&&std::abs(s.scrollTarget()-s.scroll)<.001f)s.ui.dock=false;
}
struct Camera {
 // Independent scene anchored 100 km from SA; calculations use local coordinates
 // to avoid float precision loss. No GTA entity, camera, CJ or map is moved.
 static constexpr V3 origin={100000,100000,1000};
 V3 eye={0,-12,3.4f},right={1,0,0},up,forward;float focal=1,w=1280,h=720;
 Camera(float width,float height,float span,float centerY=.48f):w(width),h(height){eye={0,-span*2.2f,span*.57f};const V3 target={0,0,span*.11f};forward=unit(target-eye);right=unit(cross(forward,{0,0,1}));up=cross(right,forward);focal=h*1.40f;cy=h*centerY;}
 float cy=0,offsetX=0,offsetY=0;
 bool project(V3 p,float& x,float& y,float& z,float& rhw)const{const V3 d=p-eye;const float depth=dot(d,forward);if(depth<=.1f)return false;rhw=1/depth;x=offsetX+w*.5f+dot(d,right)*focal*rhw;y=offsetY+cy-dot(d,up)*focal*rhw;z=1.f-.1f*rhw;return true;}
};
}
