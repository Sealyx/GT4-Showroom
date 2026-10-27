#pragma once
#include "NativePreview.hpp"
#include "ExtrasPolicy.hpp"
#include <map>
#include <set>
namespace gt4::nativeExtras {
inline int model=-1;inline std::array<int,2> choice{{0,0}};
inline std::vector<int> nativeIndices;inline std::vector<std::string> nativeLabels,vehfuncsLabels;
inline std::map<std::pair<int,int>,VehFuncsCatalogue> catalogues;
inline VehFuncsCatalogue* active=nullptr;inline VehFuncsCatalogue* probing=nullptr;
inline int probingSeed=-1;inline bool available=false,metadataReady=false,preparedScan=false;inline uint64_t nextProbe=0;
inline void scan(const Entry&e,bool force=false);
inline const char* name(void*f){return reinterpret_cast<const char*(__cdecl*)(void*)>(0x72FB30)(f);}
inline bool hasExtras(void*f,int depth=0){if(depth>64)return false;int budget=0;for(;f&&budget++<4096;f=game::field<void*>(f,0x9C)){const char*n=name(f);if(n&&std::strncmp(n,"f_extras",8)==0)return true;if(hasExtras(game::field<void*>(f,0x98),depth+1))return true;}return false;}
inline uint64_t hashBytes(uint64_t h,const void*p,size_t n){const auto*b=static_cast<const uint8_t*>(p);for(size_t i=0;i<n;++i)h=(h^b[i])*1099511628211ull;return h;}
struct Signature {std::vector<std::string> parts;bool valid=true;size_t atomics=0,bytes=0;};
inline void* __cdecl signatureAtomic(void*a,void*data){
 auto&s=*static_cast<Signature*>(data);if(++s.atomics>8192){s.valid=false;return a;}
 void*f=game::field<void*>(a,4),*g=game::field<void*>(a,0x18);if(!f||!g||!(game::field<uint8_t>(a,2)&4)||!nativePreview::frameVisible(f))return a;
 std::string path;int depth=0;for(void*n=f;n&&depth<96;n=game::field<void*>(n,4),++depth){const char*text=name(n);if(text){const size_t len=strnlen(text,128);if(len==128){s.valid=false;return a;}path.append(text,len);}path+='\x1F';}if(depth>=96){s.valid=false;return a;}
 const int vertices=game::field<int>(g,0x14),triangles=game::field<int>(g,0x10);if(vertices<0||vertices>500000||triangles<0||triangles>2000000){s.valid=false;return a;}
 uint64_t h=14695981039346656037ull;h=hashBytes(h,&vertices,sizeof(vertices));h=hashBytes(h,&triangles,sizeof(triangles));
 if(!(game::field<uint32_t>(g,8)&0x01000000)){void*m=game::field<void*>(g,0x5C);const auto*v=m?game::field<V3*>(m,0x14):nullptr;if(v&&vertices)for(int i=0;i<std::min(vertices,16);++i){const int index=int((int64_t(i)*(vertices-1))/std::max(1,std::min(vertices,16)-1));h=hashBytes(h,v+index,sizeof(V3));}}
 path+=std::to_string(h);if(path.size()>2*1024*1024-s.bytes){s.valid=false;return a;}s.bytes+=path.size();s.parts.push_back(std::move(path));return a;
}
inline std::string signature(void*v){void*c=game::field<void*>(v,0x18);if(!c)return {};Signature s;reinterpret_cast<void*(__cdecl*)(void*,void*(__cdecl*)(void*,void*),void*)>(0x749B70)(c,signatureAtomic,&s);if(!s.valid||s.parts.empty())return {};return extraOutcomeKey(std::move(s.parts));}
inline int actualSeed(void*v){auto module=GetModuleHandleW(L"VehFuncs.asi");auto get=reinterpret_cast<int(__cdecl*)(void*)>(GetProcAddress(module,"Ext_GetCarSeed"));return get?get(v):-1;}
inline void observe(void*v){
 if(!v||game::field<int16_t>(v,0x22)!=model)return;
 if(!preparedScan&&!probing){Entry e;e.id=model;scan(e,true);preparedScan=true;}
 auto*c=probing?probing:active;if(!c||(!probing&&!c->variants.empty()))return;
 const int expected=probing?probingSeed:game::previewSeed(model);if(expected<=0||actualSeed(v)!=expected){c->stopped=true;return;}
 const std::array<int8_t,2> pair{{game::field<int8_t>(v,0x438),game::field<int8_t>(v,0x439)}};
 const auto key=signature(v);if(key.empty()){c->stopped=true;return;}c->add(expected,key,pair);
}
inline void updateSelection(){
 if(available&&active){vehfuncsLabels=active->labels();if(size_t(choice[1])>=vehfuncsLabels.size())choice[1]=0;game::extraComponents=active->components;game::extrasSeed=active->seed(size_t(choice[1]),game::previewSeed(model));}
 else{vehfuncsLabels.clear();choice[1]=0;game::extrasSeed=-1;game::extraComponents={{-2,-2}};}
 game::extraChoice=choice[0]==0?-2:choice[0]==1?-1:nativeIndices[size_t(choice[0]-2)];
}
inline void activate(){
 if(!available){active=nullptr;updateSelection();return;}
 const auto key=std::make_pair(model,choice[0]);if(!catalogues.count(key)&&catalogues.size()>=128)for(auto it=catalogues.begin();it!=catalogues.end();++it)if(it->first.first!=model){catalogues.erase(it);break;}
 active=&catalogues[key];updateSelection();
}
inline const std::vector<std::string>& labels(int group){static const std::vector<std::string> empty;return group==0?nativeLabels:group==1?vehfuncsLabels:empty;}
inline void clear(){model=-1;choice={{0,0}};nativeIndices.clear();nativeLabels.clear();vehfuncsLabels.clear();active=probing=nullptr;available=metadataReady=preparedScan=false;game::extraChoice=-2;game::extrasModel=-1;game::extrasSeed=-1;game::extraComponents={{-2,-2}};nativePreview::observeVehicle=nullptr;}
inline void resetCatalogues(){clear();catalogues.clear();}
inline void scan(const Entry&e,bool force){
 if(model==e.id&&metadataReady&&!force)return;
 if(model!=e.id){clear();model=e.id;game::extrasModel=model;}
 void*mi=gta::modelInfo(model);if(!game::vehicleInfo(mi)||!gta::modelReady(model))return;
 nativeIndices.clear();nativeLabels.clear();available=false;
 void*structure=game::field<void*>(mi,0x5C);if(structure){const auto count=game::field<uint8_t>(structure,0x30C);if(count<=6)for(unsigned i=0;i<count;++i)if(game::field<void*>(structure,0x2F4+i*4))nativeIndices.push_back(int(i));}
 if(!nativeIndices.empty()){nativeLabels={"Default","None"};for(int id:nativeIndices)nativeLabels.push_back("Extra "+std::to_string(id+1));}
 VehFuncsFrames binding;binding.bind();if(binding.verified){void*c=game::field<void*>(mi,0x1C);available=c&&hasExtras(game::field<void*>(c,4));if(!available&&structure)for(int id:nativeIndices){void*a=game::field<void*>(structure,0x2F4+size_t(id)*4);if(a&&hasExtras(game::field<void*>(a,4)))available=true;}}
 if(choice[0]<0||size_t(choice[0])>=nativeLabels.size())choice[0]=0;
 metadataReady=true;activate();nativePreview::observeVehicle=observe;
}
inline bool select(int group,int option){const auto&options=labels(group);if(group<0||group>1||option<0||size_t(option)>=options.size())return false;const bool changed=choice[size_t(group)]!=option;choice[size_t(group)]=option;if(group==0&&changed)activate();else updateSelection();return true;}

inline bool pump(uint64_t now){
 if(!active||active->stopped||active->variants.empty()||now<nextProbe)return false;nextProbe=now+16;
 probing=active;probingSeed=variantProbeSeed(model,active->samples);const auto before=active->variants.size();const int saved=game::extrasSeed;game::extrasSeed=probingSeed;
 const bool ok=nativePreview::inspect(model);game::extrasSeed=saved;probing=nullptr;probingSeed=-1;if(!ok)active->stopped=true;updateSelection();return active->variants.size()!=before;
}
inline std::unique_ptr<Mesh> capture(int id){if(id==model)preparedScan=false;auto mesh=nativePreview::snapshot(id);updateSelection();return mesh;}
inline bool apply(void*v){
 if(!v||game::field<int16_t>(v,0x22)!=model||!available||!active||active->variants.empty())return true;

 return nativePreview::initialize(v,model);
}
}
