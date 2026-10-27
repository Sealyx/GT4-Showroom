#pragma once
#include "SpawnEntry.hpp"
namespace gt4 {
struct OutcomeEntityRef {
 uintptr_t poolAddress=0,objects=0,flags=0;int index=-1;uint8_t generation=0;size_t stride=0;
 struct Pool {uintptr_t objects,flags;int count;};
 static bool pool(uintptr_t address,Pool&out){uintptr_t p=0;return gta::read(address,p)&&p&&gta::read(p,out)&&out.objects&&out.flags&&out.count>0&&out.count<=100000;}
 static OutcomeEntityRef from(void*entity,bool vehicle=false){
  OutcomeEntityRef out;out.poolAddress=vehicle?0xB74494:0xB74490;out.stride=vehicle?0xA18:0x7C4;Pool p{};const auto raw=uintptr_t(entity);
  if(!entity||!pool(out.poolAddress,p)||raw<p.objects)return {};
  const auto offset=raw-p.objects;if(offset%out.stride||offset/out.stride>=size_t(p.count))return {};
  out.index=int(offset/out.stride);if(!gta::read(p.flags+size_t(out.index),out.generation)||(out.generation&0x80))return {};
  out.objects=p.objects;out.flags=p.flags;return out;
 }
 void*get()const{Pool p{};uint8_t flag=0;if(index<0||!pool(poolAddress,p)||p.objects!=objects||p.flags!=flags||index>=p.count||!gta::read(flags+size_t(index),flag)||flag!=generation||(flag&0x80))return nullptr;return reinterpret_cast<void*>(objects+size_t(index)*stride);}
};
inline EntryCameraPose outcomeCameraPose(double age,V3 cj,V3 direction,float yaw){
 const float orbit=.13f*std::sin(float(age)*.34f),a=yaw+orbit;
 EntryCameraPose p;p.fov=55;p.source=cj+V3{4.3f*std::cos(a),4.3f*std::sin(a),2.15f+.1f*std::sin(float(age)*.45f)};
 const float n=std::sqrt(direction.x*direction.x+direction.y*direction.y);if(n>.01f)direction=direction*(.2f/n);else direction={};
 p.target=cj+V3{direction.x,direction.y,.18f};return p;
}
struct OutcomeCamera {
 OutcomeEntityRef player,actor;bool owned=false,busted=false;double age=0;float yaw=0;V3 source{},target{};float desiredFov=55;
 void*camera()const{return reinterpret_cast<void*>(0xB6F028);}
 bool owns()const{if(!owned||game::field<uint32_t>(camera(),0xB0)!=1||!game::field<uint8_t>(camera(),0x2C))return false;const V3 a=game::field<V3>(camera(),0x86C)-source,b=game::field<V3>(camera(),0x860)-target;return dot(a,a)<.01f&&dot(b,b)<.01f;}
 void stop(bool reset=false){const bool restore=owns();owned=false;player={};actor={};if(restore&&!game::loading()&&OutcomeEntityRef::from(entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1)).get())entryNative::function<void(__thiscall*)(void*)>(reset?0x50BAB0:0x50B930)(camera());}
 static OutcomeEntityRef culprit(void*p,bool arrest){
  if(!arrest){void*damager=nullptr;if(gta::read(uintptr_t(p)+0x764,damager)&&damager!=p){auto ref=OutcomeEntityRef::from(damager);if(!ref.get())ref=OutcomeEntityRef::from(damager,true);if(ref.get())return ref;}}
  if(arrest){OutcomeEntityRef::Pool pool{};if(OutcomeEntityRef::pool(0xB74490,pool)){float best=30.f*30.f;OutcomeEntityRef found;const V3 center=game::position(p);for(int i=0;i<std::min(pool.count,4096);++i){uint8_t flag=0;if(!gta::read(pool.flags+size_t(i),flag)||(flag&0x80))continue;void*other=reinterpret_cast<void*>(pool.objects+size_t(i)*0x7C4);int type=0;float health=0;if(other==p||!gta::read(uintptr_t(other)+0x598,type)||type!=6||!gta::read(uintptr_t(other)+0x540,health)||health<=0)continue;auto ref=OutcomeEntityRef::from(other);if(!ref.get())continue;const V3 delta=game::position(other)-center;const float distance=dot(delta,delta);if(distance<best){best=distance;found=ref;}}return found;}}
  return {};
 }
 V3 safe(V3 aim,V3 from,void*ignore){SpawnEntry helper;return helper.collisionSafe(aim,from,ignore);}
 bool begin(bool arrest){
  stop();void*p=entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1);player=OutcomeEntityRef::from(p);if(!player.get()||game::field<uint32_t>(camera(),0xB0)==1)return false;
  busted=arrest;age=0;actor=culprit(p,arrest);V3 cj=game::position(p),direction{};if(void*a=actor.get())direction=game::position(a)-cj;
  yaw=dot(direction,direction)>.01f?std::atan2(direction.y,direction.x)+.85f:game::field<float>(p,0x558)+.8f;
  float best=-1;const float start=yaw;for(float offset:{0.f,1.57f,-1.57f,3.14f}){auto pose=outcomeCameraPose(0,cj,direction,start+offset);const V3 checked=safe(pose.target,pose.source,p);const float span=dot(checked-pose.target,checked-pose.target);if(span>best){best=span;yaw=start+offset;}}
  auto pose=outcomeCameraPose(0,cj,direction,yaw);source=safe(pose.target,pose.source,p);target=pose.target;V3 up{};entryNative::function<void(__thiscall*)(void*,const V3*,const V3*)>(0x50BEC0)(camera(),&source,&up);entryNative::function<void(__thiscall*)(void*,const V3*,int16_t,int)>(0x50C8B0)(camera(),&target,2,1);owned=true;if(!owns()){owned=false;return false;}return true;
 }
 bool update(double dt,bool paused){
  if(!owned)return false;void*p=player.get();if(!p||p!=entryNative::function<void*(__cdecl*)(int)>(0x56E210)(-1)||game::loading()||!owns()){stop();return false;}if(paused)return true;
  age+=std::clamp(dt,0.,.1);const V3 cj=game::position(p);V3 direction{};if(void*a=actor.get()){direction=game::position(a)-cj;if(dot(direction,direction)>1600.f)direction={};}
  const auto pose=outcomeCameraPose(age,cj,direction,yaw);const float ease=1.f-std::exp(-float(std::clamp(dt,0.,.1))*7.f);source=source+(pose.source-source)*ease;target=pose.target;source=safe(target,source,p);const V3 up{};
  entryNative::function<void(__thiscall*)(void*,const V3*,const V3*)>(0x50BEC0)(camera(),&source,&up);game::field<V3>(camera(),0x860)=target;return true;
 }
};
}
