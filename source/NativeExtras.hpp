#pragma once
#include "NativePreview.hpp"
#include <set>
#include "ExtrasPolicy.hpp"
namespace gt4::nativeExtras {
struct Group {void* frame=nullptr;std::string name;std::vector<void*> children;std::vector<std::string> names;};
inline int model=-1;inline std::array<int,2> choice{{0,0}};inline std::vector<int> nativeIndices;inline std::vector<std::string> nativeLabels,vehfuncsLabels;inline std::vector<unsigned> variantCounts;
inline const char* name(void*f){return reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(f);}
inline void groups(void*f,std::vector<Group>&out,int depth=0){
 if(depth>64||out.size()>=8)return;int budget=0;for(;f&&budget++<4096;f=game::field<void*>(f,0x9C)){
  const char*n=name(f);const std::string key=n?lowerName(n):"";void*child=game::field<void*>(f,0x98);
  if(key.rfind("f_extras",0)==0){Group g;g.frame=f;g.name=n;int count=0;for(void*c=child;c&&count++<16;c=game::field<void*>(c,0x9C)){const char*cn=name(c);if(cn&&std::strncmp(cn,"f_class",7)==0){g.children.clear();break;}g.children.push_back(c);g.names.push_back(cn?cn:"");}if(!g.children.empty())out.push_back(std::move(g));}
  else if(child)groups(child,out,depth+1);
 }
}
inline std::vector<std::string> labels(int group){return group==0?nativeLabels:group==1?vehfuncsLabels:std::vector<std::string>{};}
inline void clear(){model=-1;choice={{0,0}};nativeIndices.clear();nativeLabels.clear();vehfuncsLabels.clear();variantCounts.clear();game::extraChoice=-2;game::extrasModel=-1;game::extrasSeed=-1;nativePreview::prepareExtras=nullptr;}
inline void apply(void* vehicle){
 if(!vehicle||game::field<int16_t>(vehicle,0x22)!=model||!choice[1])return;VehFuncsFrames binding;binding.bind();if(!binding.verified)return;
 auto toggle=reinterpret_cast<void(__cdecl*)(void*,bool)>(GetProcAddress(GetModuleHandleW(L"VehFuncs.asi"),"Ext_ToggleFrameHide"));if(!toggle)return;
 std::vector<Group> available;void*clump=game::field<void*>(vehicle,0x18);if(clump)groups(game::field<void*>(clump,4),available);if(available.size()!=variantCounts.size())return;
 const auto combination=extraCombination(choice[1]>=2?unsigned(choice[1]-2):0,variantCounts);if(combination.size()!=variantCounts.size())return;
 for(size_t g=0;g<available.size();++g){auto&group=available[g];if(group.children.size()!=variantCounts[g])return;const size_t selected=combination[g];for(size_t i=0;i<group.children.size();++i)toggle(group.children[i],choice[1]==1||i!=selected);}
}
inline void scan(const Entry&e){
 if(model==e.id)return;clear();model=e.id;game::extrasModel=model;void*mi=gta::modelInfo(model);if(!game::vehicleInfo(mi)||!gta::modelReady(model))return;
 void*structure=game::field<void*>(mi,0x5C);if(structure){const auto count=game::field<uint8_t>(structure,0x30C);if(count<=6)for(unsigned i=0;i<count;++i)if(game::field<void*>(structure,0x2F4+i*4))nativeIndices.push_back(int(i));}
 if(!nativeIndices.empty()){nativeLabels={"Default","None"};for(int id:nativeIndices)nativeLabels.push_back("Extra "+std::to_string(id+1));}
 VehFuncsFrames binding;binding.bind();if(binding.verified){void*clump=game::field<void*>(mi,0x1C);std::vector<Group> available;if(clump)groups(game::field<void*>(clump,4),available);std::vector<std::vector<std::string>> names;for(const auto&g:available)names.push_back(g.names);vehfuncsLabels=extraCombinationLabels(names);if(!vehfuncsLabels.empty())for(const auto&g:available)variantCounts.push_back(unsigned(g.children.size()));}

 nativePreview::prepareExtras=apply;
}
inline bool select(int group,int option){const auto options=labels(group);if(group<0||group>1||option<0||size_t(option)>=options.size())return false;choice[size_t(group)]=option;game::extraChoice=choice[0]==0?-2:choice[0]==1?-1:nativeIndices[size_t(choice[0]-2)];game::extrasSeed=choice[1]?0:-1;return true;}
inline std::unique_ptr<Mesh> capture(int id){return nativePreview::snapshot(id);}
}
