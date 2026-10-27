#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace gt4 {
 
inline bool nativeKeyDown(const void* keyboard,int vk){
 if(!keyboard||vk<1||vk>255)return false;
 const auto* bytes=static_cast<const uint8_t*>(keyboard);
 auto at=[&](size_t offset){int16_t value=0;std::memcpy(&value,bytes+offset,sizeof(value));return value!=0;};
 if(vk>=0x70&&vk<=0x7B)return at(size_t(vk-0x70)*2);  
 switch(vk){
 case 0x1B:return at(0x218);case 0x2D:return at(0x21A);case 0x2E:return at(0x21C);
 case 0x24:return at(0x21E);case 0x23:return at(0x220);case 0x21:return at(0x222);case 0x22:return at(0x224);
 case 0x26:return at(0x226);case 0x28:return at(0x228);case 0x25:return at(0x22A);case 0x27:return at(0x22C);
 case 0x08:return at(0x254);case 0x09:return at(0x256);case 0x0D:return at(0x23C)||at(0x25A);
 case 0x10:return at(0x25C)||at(0x25E);case 0x11:return at(0x262)||at(0x264);
 case 0xA0:return at(0x25C);case 0xA1:return at(0x25E);case 0xA2:return at(0x262);case 0xA3:return at(0x264);
 case 0x12:return at(0x266)||at(0x268);
 default:
   
  if(vk<0x20||(vk>=0x7C&&vk<=0x87))return false;
  return at(0x18+size_t(vk)*2);
 }
}
struct OpenGate {
 int gameState=0,enex=0;bool loadingData=false,menu=false,codePause=false,userPause=false,focus=false,alive=false,cutscene=false,inVehicle=false;
 bool blocked()const{return gameState!=9||loadingData||menu||codePause||!focus||cutscene||inVehicle;}
 bool ready()const{return !blocked()&&!userPause&&alive&&!cutscene&&enex==0;}
};
 
 
struct FunctionKeyClaim {
 uint16_t mask=0;
 void claim(int vk){if(vk>=0x70&&vk<=0x7B)mask|=uint16_t(1u<<(vk-0x70));}
 void filter(void* keyboard,int vk,bool take,bool down){if(!keyboard||vk<0x70||vk>0x7B)return;const auto bit=uint16_t(1u<<(vk-0x70));if(take)claim(vk);if(!down){mask&=uint16_t(~bit);return;}if(mask&bit){const int16_t zero=0;std::memcpy(static_cast<uint8_t*>(keyboard)+size_t(vk-0x70)*2,&zero,2);}}
 template<class Down>void apply(void* keyboard,Down down){for(int i=0;i<12;++i)if(mask&(1u<<i))filter(keyboard,0x70+i,false,down(0x70+i));}
};
 
 
struct ScopedEscapePulse {
 uint8_t* current;uint8_t* previous;int16_t savedCurrent=0,savedPrevious=0;bool active;
 ScopedEscapePulse(void*now,void*old,bool enabled):current(static_cast<uint8_t*>(now)),previous(static_cast<uint8_t*>(old)),active(enabled&&now&&old){
  if(active){std::memcpy(&savedCurrent,current+0x218,2);std::memcpy(&savedPrevious,previous+0x218,2);const int16_t one=1,zero=0;std::memcpy(current+0x218,&one,2);std::memcpy(previous+0x218,&zero,2);}
 }
 ~ScopedEscapePulse(){if(active){int16_t n=0,o=0;std::memcpy(&n,current+0x218,2);std::memcpy(&o,previous+0x218,2);if(n==1&&o==0){std::memcpy(current+0x218,&savedCurrent,2);std::memcpy(previous+0x218,&savedPrevious,2);}}}
};

}
