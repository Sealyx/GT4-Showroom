#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>
namespace gt4 {
// Canonical bounded fingerprints keep discovery storage independent of mesh size.
inline std::string extraOutcomeKey(std::vector<std::string> parts){if(parts.empty()||parts.size()>8192)return {};size_t bytes=0;for(const auto&p:parts){if(p.size()>2*1024*1024-bytes)return {};bytes+=p.size();}std::sort(parts.begin(),parts.end());uint64_t a=14695981039346656037ull,b=0x9E3779B97F4A7C15ull;for(const auto&p:parts){for(unsigned char c:p){a=(a^c)*1099511628211ull;b^=uint64_t(c)+0x9E3779B97F4A7C15ull+(b<<6)+(b>>2);}a=(a^255)*1099511628211ull;b^=p.size()+0x517CC1B727220A95ull+(b<<6)+(b>>2);}return std::to_string(parts.size())+":"+std::to_string(a)+":"+std::to_string(b);}
// Store only complete native outcomes. Default is outcome zero, never an
// additional layer of geometry to combine with a selected alternative.
struct VehFuncsVariant {int seed=1;std::string signature;};
struct VehFuncsCatalogue {
 static constexpr size_t maxVariants=64,maxSamples=192,quietSamples=48;
 std::vector<VehFuncsVariant> variants;std::array<int8_t,2> components{{-2,-2}};
 size_t samples=0,quiet=0;bool stopped=false;
 bool add(int seed,std::string signature,std::array<int8_t,2> pair){
  if(seed<=0||signature.empty()||stopped)return false;
  if(!variants.empty()&&pair!=components){stopped=true;return false;}
  ++samples;
  for(const auto&v:variants)if(v.signature==signature){++quiet;finish();return false;}
  if(variants.empty())components=pair;
  variants.push_back({seed,std::move(signature)});quiet=0;finish();return true;
 }
 void finish(){stopped=samples>=maxSamples||quiet>=quietSamples||variants.size()>=maxVariants;}
 int seed(size_t option,int fallback)const{return option<variants.size()?variants[option].seed:fallback;}
 std::vector<std::string> labels()const{std::vector<std::string> out{"Default"};for(size_t i=1;i<variants.size();++i)out.push_back("Extras "+std::to_string(i));return out;}
};
inline int variantProbeSeed(int model,size_t sample){uint32_t x=uint32_t(model)^uint32_t(sample+1)*0x9E3779B9u;x^=x>>16;x*=0x7FEB352Du;x^=x>>15;x*=0x846CA68Bu;x^=x>>16;return int((x&0x7FFFFFFFu)|1u);}
}
