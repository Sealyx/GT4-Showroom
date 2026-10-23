#pragma once
#include "InputState.hpp"
namespace gt4 {
enum class SearchAction{None,Submit,Cancel};
struct SearchInput{
 std::array<Key,256> keys{};
 void quarantine(){for(auto&k:keys)k.blockUntilRelease();}
 template<class Down>SearchAction poll(Down down,uint64_t tick,std::string& text){SearchAction action=SearchAction::None;for(int vk=8;vk<256;++vk){const bool event=keys[size_t(vk)].poll(down(vk),tick,vk==8||vk==0x2E);if(!event)continue;if(vk==13){action=SearchAction::Submit;continue;}if(vk==8){if(down(0x11))text.clear();else if(!text.empty())text.pop_back();else action=SearchAction::Cancel;continue;}if(vk==0x2E){text.clear();continue;}if(down(0x11)||down(0x12))continue;char c=0;if((vk>='A'&&vk<='Z')||(vk>='0'&&vk<='9'))c=char(vk);else if(vk>=0x60&&vk<=0x69)c=char('0'+vk-0x60);else if(vk==32)c=' ';else if(vk==0xBD)c=down(0x10)?'_':'-';else if(vk==0xBE||vk==0x6E)c='.';if(c&&text.size()<48)text+=c;}return action;}
};
}
