#pragma once
#include <string>
#include <vector>
#include <cstddef>
namespace gt4 {
inline std::vector<std::string> extraCombinationLabels(const std::vector<std::vector<std::string>>&groups){
 if(groups.empty()||groups.size()>8)return {};size_t total=1;for(const auto&g:groups){if(g.empty()||g.size()>16||total>64/g.size())return {};total*=g.size();}
 std::vector<std::string> labels{"Default","None"};for(size_t i=0;i<total;++i){size_t n=i;std::string label;for(const auto&g:groups){const auto part=n%g.size();n/=g.size();if(!label.empty())label+=" / ";label+=g[part].empty()?"Extra "+std::to_string(part+1):g[part];}labels.push_back(std::move(label));}return labels;
}
inline std::vector<size_t> extraCombination(size_t index,const std::vector<unsigned>&counts){std::vector<size_t> parts;for(unsigned n:counts){if(!n)return {};parts.push_back(index%n);index/=n;}if(index)return {};return parts;}
}
