#pragma once
#include <string>
#include <cctype>
#include <cstdint>
#include <cstddef>
#include <utility>
namespace gt4 {
inline bool previewHighDetail(const char*node,uintptr_t callback=0){
 if(callback==0x7331E0||callback==0x732820||callback==0x7334F0)return false;
 if(!node)return true;std::string name=node;for(auto&c:name)c=char(std::tolower(static_cast<unsigned char>(c)));
 for(const char*tag:{"_vlo","_lod","_low","_lo","_dam"}){size_t p=0;const std::string t=tag;while((p=name.find(t,p))!=std::string::npos){const size_t end=p+t.size();if(end==name.size()||name[end]=='_'||std::isdigit(static_cast<unsigned char>(name[end])))return false;p=end;}}
 return true;
}
template<class Emit> size_t previewTriangles(const uint16_t* indices,size_t count,
 uint32_t primitive,size_t vertices,size_t budget,Emit emit){
 if(!indices||count>2000000||vertices<3||vertices>500000||!budget||(primitive!=0&&primitive!=1))return 0;
 size_t emitted=0;
 auto triangle=[&](uint16_t a,uint16_t b,uint16_t c){
  if(a>=vertices||b>=vertices||c>=vertices||a==b||b==c||a==c)return;
  if(emitted<budget){emit(a,b,c);++emitted;}
 };
 if(primitive==0){for(size_t i=0;i+2<count&&emitted<budget;i+=3)triangle(indices[i],indices[i+1],indices[i+2]);}
 else{for(size_t i=2;i<count&&emitted<budget;++i){
  if(i&1)triangle(indices[i-1],indices[i-2],indices[i]);
  else triangle(indices[i-2],indices[i-1],indices[i]);
 }}
 return emitted;
}
inline unsigned textureAddress(unsigned mode,unsigned fallback=1){return mode>=1&&mode<=4?mode:fallback;}
inline std::pair<unsigned,unsigned> textureAddresses(uint32_t flags){const auto u=textureAddress((flags>>8)&15);return {u,textureAddress((flags>>12)&15,u)};}
}
