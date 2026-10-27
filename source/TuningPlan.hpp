#pragma once
#include <map>
#include <set>
#include <vector>
#include <random>
#include <cstdint>
namespace gt4 {
struct TuningOption {int id=-1,slot=-1;bool effect=false;}; 
inline std::vector<int> chooseTuning(const std::vector<TuningOption>& options,uint32_t serial,int style){
 std::map<int,std::vector<int>> groups;std::set<int> seen;
 for(const auto& o:options)if(o.id>=0&&o.slot>=0&&o.slot<128&&seen.insert(o.id).second)groups[o.slot].push_back(o.id);
 std::mt19937 rng(serial^0x4B495431u);std::vector<int> result;
 for(const auto& group:groups){const auto& choices=group.second;const size_t choice=group.first==34||choices.size()!=2?std::uniform_int_distribution<size_t>(0,choices.size()-1)(rng):size_t(style)%choices.size();result.push_back(choices[choice]);}
 return result;
}
inline int tuningSlot(uint16_t flags){return flags&0x8000?int((flags>>10)&31)+((flags&0x100)?32:0):-1;}
inline bool tuningEffect(int slot){return slot>=15&&slot<=17;}
inline constexpr int prizeStreamingFlags=8;  
}
