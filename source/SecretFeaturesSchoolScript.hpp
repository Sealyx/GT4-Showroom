#pragma once
#include "SecretFeaturesState.hpp"
#include "SecretFeaturesKnownPatches.hpp"
#include <array>
#include <cstring>
#include <cstddef>
namespace gt4 {
struct SecretFeaturesProfile {
 static constexpr size_t bytes=46648;static constexpr uint32_t fnv=0x68EAB6D8;
 static constexpr size_t resultSub=33258,resultEnd=35069,choiceButton=35073;
 static constexpr size_t testGlobal=988,scoreGlobal=1084,leaveGlobal=1300,confirmGlobal=1304;
 static uint32_t hash(const uint8_t*b,size_t n){uint32_t h=2166136261u;for(size_t i=0;i<n;++i)h=(h^b[i])*16777619u;return h;}
 static bool match(const uint8_t*b,size_t n){if(!b||n<bytes)return false;if(hash(b,bytes)==fnv)return true;std::array<uint8_t,bytes> normalized{};std::memcpy(normalized.data(),b,bytes);return SecretFeaturesKnownPatches::silentPatch(normalized.data(),normalized.size())&&hash(normalized.data(),normalized.size())==fnv;}
};
struct SecretFeaturesCommand {
 uintptr_t owner=0,base=0,ip=0;const uint8_t*mission=nullptr;size_t length=0;bool active=false,external=false,isMission=false;
 std::array<char,8> name{};int test=0,score=100,leave=0,testRunning=0;
};
struct SecretFeaturesSkip {bool consume=false,compareFalse=false;size_t next=0;};
inline size_t secretFeaturesArgumentEnd(const uint8_t*b,size_t n,size_t at,int count){
 for(int j=0;j<count;++j){if(at>=n)return 0;const unsigned t=b[at++];size_t size=0;switch(t){case 1:case 6:size=4;break;case 2:case 3:case 5:case 10:case 11:case 16:case 17:size=2;break;case 4:size=1;break;case 7:case 8:case 12:case 13:case 18:case 19:size=6;break;case 9:size=8;break;case 15:size=16;break;case 14:if(at>=n)return 0;size=1+b[at];break;default:return 0;}if(size>n-at)return 0;at+=size;}return at;
}
class SecretFeaturesSchoolScript {
 uintptr_t verifiedBase=0,owner=0,rejectedBase=0;uint64_t touched=0;bool latched=false;bool verified=false,testArmed=false,recordStopped=false;uint64_t observed=0;
public:
 SecretFeaturesState state;bool enabled=true,assetsReady=false;bool mismatch=false;bool recording=false;uint64_t recordSerial=0;
 uintptr_t ownerAddress()const{return owner;}
 bool owns(uintptr_t p)const{return verified&&owner==p;}
 bool owns(uintptr_t p,uintptr_t base)const{return verified&&owner==p&&verifiedBase==base;}
 bool isVerified()const{return verified;}
 bool needsScene()const{return recording||(testArmed&&recordStopped)||state.active;}
 void reset(){verifiedBase=owner=rejectedBase=0;touched=0;latched=false;verified=false;testArmed=recordStopped=recording=false;observed=0;recordSerial=0;mismatch=false;state.reset();}
 void stop(){owner=verifiedBase=0;verified=false;touched=observed=0;latched=testArmed=recordStopped=recording=false;state.reset();}
 void suspended(uint64_t now){if(verified){if(touched)touched=now;if(observed)observed=now;}}
 void expire(uint64_t now){const uint64_t latest=std::max(touched,observed);if(latest&&now-latest>500)stop();}
 SecretFeaturesSkip inspect(const SecretFeaturesCommand&c,uint64_t now){
  SecretFeaturesSkip out;if(!enabled||!assetsReady||!c.active||c.external||!c.isMission||std::memcmp(c.name.data(),"DSKOOL\0\0",8))return out;
  if(c.base!=verifiedBase){
   if(c.base==rejectedBase)return out;verified=SecretFeaturesProfile::match(c.mission,c.length);
   if(!verified){rejectedBase=c.base;mismatch=true;return out;}verifiedBase=c.base;mismatch=false;
  }
  if(!verified||c.ip<c.base||c.ip-c.base+2>SecretFeaturesProfile::bytes)return out;
  observed=now;const size_t at=size_t(c.ip-c.base);const auto*b=c.mission;const uint16_t op=uint16_t(b[at]|unsigned(b[at+1])<<8)&0x7FFF;
  if(op==0x0050&&at+7<=SecretFeaturesProfile::bytes&&b[at+2]==1){int32_t target=0;std::memcpy(&target,b+at+3,4);
   if(target==-31041){state.reset();latched=false;owner=c.owner;testArmed=true;recordStopped=false;recording=false;++recordSerial;}
   if(target==-32505){recordStopped=true;recording=false;}
   if(target==-int32_t(SecretFeaturesProfile::resultSub)){
   if(c.score<70&&c.test>=1&&c.test<=16&&c.leave==0){touched=now;owner=c.owner;if(!latched){state.begin(c.test);latched=true;}}
   else{testArmed=recordStopped=recording=false;if(state.active){state.reset();latched=false;owner=0;}}
  }}
  recording=testArmed&&!recordStopped&&!state.active&&c.testRunning==1&&c.leave==0;
  if(!state.active||owner!=c.owner)return out;
  if(at>=SecretFeaturesProfile::resultSub&&at<SecretFeaturesProfile::resultEnd){
   int args=-1;switch(op){case 0x0937:args=6;break;case 0x033E:args=3;break;case 0x045A:args=4;break;case 0x045B:args=5;break;case 0x038D:args=9;break;default:break;}
   if(args>=0){const size_t end=secretFeaturesArgumentEnd(b,SecretFeaturesProfile::bytes,at+2,args);if(end){out.consume=true;out.next=end;}}
  }
  if(at==SecretFeaturesProfile::choiceButton&&op==0x00E1){out.next=secretFeaturesArgumentEnd(b,SecretFeaturesProfile::bytes,at+2,2);out.consume=out.next!=0;out.compareFalse=out.consume;}
  return out;
 }
 template<class Write>bool choose(SecretFeaturesChoice choice,Write write){
  if(choice==SecretFeaturesChoice::None||!verified||!state.active||!state.accepted||!owner)return false;
  if(choice==SecretFeaturesChoice::Exit&&!write(SecretFeaturesProfile::leaveGlobal,2))return false;
  return write(SecretFeaturesProfile::confirmGlobal,1);
 }
};
}
