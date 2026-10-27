#pragma once
#include <cstdint>
#include <array>
#include <cstring>
#include <string>
namespace gt4 {
struct Key {
 bool held=false;uint64_t next=0;
 bool poll(bool down,uint64_t now,bool repeat=false){if(!down){held=false;return false;}if(!held){held=true;next=now+420;return true;}if(repeat&&now>=next){next=now+105;return true;}return false;}
 void blockUntilRelease(){held=true;next=UINT64_MAX;}
};
struct ControllerInput {
 std::array<int16_t,24> values{};
 void capture(const void* raw){std::memcpy(values.data(),raw,sizeof(values));}
 bool down(size_t offset)const{return !(offset&1)&&offset/2<values.size()&&values[offset/2]!=0;}
};
struct ExitButtons {
 uint32_t mask=0;
 void arm(const int16_t* state){for(unsigned i=4;i<24;++i)if(state[i])mask|=1u<<i;}
 void filter(int16_t* state){for(unsigned i=4;i<24;++i)if(mask&(1u<<i)){if(state[i])state[i]=0;else mask&=~(1u<<i);}}
};
struct OpenIntent {
 uint64_t until=0;
 void request(uint64_t now){until=now+2000;}
 bool pending(uint64_t now)const{return until&&now<=until;}
 void clear(){until=0;}
};
struct TypedReload {
 std::array<bool,256> held{};std::string text;uint64_t last=0;
 bool feed(char c,uint64_t tick){if(last&&tick-last>3000)text.clear();last=tick;text+=c;if(text.size()>7)text.erase(0,text.size()-7);if(text=="GTSPAWN"){text.clear();return true;}return false;}
 template<class Down>bool poll(Down down,uint64_t tick,bool active){bool trigger=false;for(int vk=8;vk<=90;++vk){if(vk<32&&vk!=8)continue;const bool pressed=down(vk);if(active&&pressed&&!held[size_t(vk)]){if(vk>='A'&&vk<='Z')trigger=feed(char(vk),tick)||trigger;else text.clear();}held[size_t(vk)]=pressed;}if(!active)text.clear();return trigger;}
};
struct MoveCueGate{uint64_t next=0;bool allow(uint64_t tick){if(tick<next)return false;next=tick+100;return true;}};
struct PauseOwnership {
 bool owned=false;
 template<class Start>void begin(bool paused,Start start){if(!paused){start();owned=true;}}
 template<class Start>void maintain(bool paused,bool menu,bool codePause,Start start){if(owned&&!paused&&!menu&&!codePause)start();}
 template<class End>void end(bool menu,End finish){if(owned&&!menu)finish();owned=false;}
};
}
