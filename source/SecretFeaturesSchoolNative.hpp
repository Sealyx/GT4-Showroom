#pragma once
#include "SecretFeaturesSchoolScript.hpp"
#include "RuntimeModels.hpp"
#include <windows.h>
namespace gt4 {
class SecretFeaturesSchoolNative {
 uintptr_t copiedBase=0;std::array<uint8_t,SecretFeaturesProfile::bytes> copy{};
 static constexpr uintptr_t globals=0xA49960;
 static bool writable(uintptr_t address,size_t size){MEMORY_BASIC_INFORMATION i{};if(!VirtualQuery(reinterpret_cast<void*>(address),&i,sizeof(i))||i.State!=MEM_COMMIT||(i.Protect&(PAGE_GUARD|PAGE_NOACCESS)))return false;const DWORD p=i.Protect&255;return (p==PAGE_READWRITE||p==PAGE_WRITECOPY||p==PAGE_EXECUTE_READWRITE||p==PAGE_EXECUTE_WRITECOPY)&&address>=uintptr_t(i.BaseAddress)&&size<=i.RegionSize-(address-uintptr_t(i.BaseAddress));}
 bool snapshot(void*self,SecretFeaturesCommand&c){
  const uintptr_t s=uintptr_t(self);c.owner=s;if(!gta::reader(s+8,c.name.data(),8)||std::memcmp(c.name.data(),"DSKOOL\0\0",8))return false;
  uint8_t active=0,external=0,mission=0;uintptr_t ip=0;
  if(!gta::read(s+0x10,c.base)||!gta::read(s+0x14,ip)||ip<2||!gta::read(s+0xC4,active)||!gta::read(s+0xC7,external)||!gta::read(s+0xDC,mission))return false;
  c.active=active!=0;c.external=external!=0;c.isMission=mission!=0;c.ip=ip-2;if(!c.active||c.external||!c.isMission)return false;
  if(c.base!=copiedBase){if(!gta::reader(c.base,copy.data(),copy.size()))return false;copiedBase=c.base;}
  c.mission=copy.data();c.length=copy.size();return gta::read(globals+SecretFeaturesProfile::testGlobal,c.test)&&gta::read(globals+SecretFeaturesProfile::scoreGlobal,c.score)&&gta::read(globals+SecretFeaturesProfile::leaveGlobal,c.leave)&&gta::read(globals+996,c.testRunning);
 }
public:
 SecretFeaturesSchoolScript script;
 void reset(){script.reset();copiedBase=0;}
 bool command(void*self,int command,uint64_t now){
  if(!script.enabled||!script.assetsReady)return false;
  if(command!=0x0050&&command!=0x00E1&&command!=0x0937&&command!=0x033E&&command!=0x045A&&command!=0x045B&&command!=0x038D)return false;
  SecretFeaturesCommand c;if(!snapshot(self,c))return false;
  const size_t at=c.ip>=c.base?size_t(c.ip-c.base):copy.size();int args=0;switch(command){case 0x0050:args=1;break;case 0x00E1:args=2;break;case 0x0937:args=6;break;case 0x033E:args=3;break;case 0x045A:args=4;break;case 0x045B:args=5;break;case 0x038D:args=9;break;default:return false;}
  if(at+2>copy.size())return false;const size_t end=secretFeaturesArgumentEnd(copy.data(),copy.size(),at+2,args);std::array<uint8_t,256>live{};
  if(!end||end-at>live.size()||!gta::reader(c.ip,live.data(),end-at)||std::memcmp(live.data(),copy.data()+at,end-at)){reset();script.mismatch=true;return false;}
  const auto skip=script.inspect(c,now);if(!skip.consume)return false;
  const uintptr_t next=c.base+skip.next;if(!writable(uintptr_t(self)+0x14,sizeof(uintptr_t)))return false;std::memcpy(static_cast<uint8_t*>(self)+0x14,&next,sizeof(next));
  if(skip.compareFalse)reinterpret_cast<void(__thiscall*)(void*,bool)>(0x4859D0)(self,false);
  return true;
 }
 bool choose(SecretFeaturesChoice choice){
  if(!script.state.active||!writable(globals+SecretFeaturesProfile::leaveGlobal,sizeof(int))||!writable(globals+SecretFeaturesProfile::confirmGlobal,sizeof(int)))return false;SecretFeaturesCommand c;
  if(!snapshot(reinterpret_cast<void*>(script.ownerAddress()),c)||!script.owns(c.owner,c.base)||!gta::reader(c.base,copy.data(),copy.size())||!SecretFeaturesProfile::match(copy.data(),copy.size())||c.test!=script.state.test||c.score>=70||c.leave!=0)return false;
  return script.choose(choice,[&](size_t offset,int value){if(!writable(globals+offset,sizeof(value)))return false;std::memcpy(reinterpret_cast<void*>(globals+offset),&value,sizeof(value));return true;});
 }
};
}
