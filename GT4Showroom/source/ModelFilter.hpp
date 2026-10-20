#pragma once
#include <set>
#include <string>
#include <charconv>
#include <cctype>
#include <vector>
#include <algorithm>
#include "ModelNames.hpp"
namespace gt4 {
// Explicit IDs or exact model/display names; no runtime vehicle-type bans.
inline constexpr const char* filterKeys[]={"Trailers","Planes","Helicopters","Boats","Custom","Trains"};
inline constexpr const char* filterDefaults[]={
 "435,450,584,591,606,607,608,610,611",
 "460,464,476,511,512,513,519,520,539,553,577,592,593",
 "417,425,447,465,469,487,488,497,501,548,563",
 "430,446,452,453,454,472,473,484,493,595","","449,537,538,569,570,590"};
inline constexpr const char* allExcludedDefaults="435,450,584,591,606,607,608,610,611,449,537,538,569,570,590,460,464,476,511,512,513,519,520,539,553,577,592,593,417,425,447,465,469,487,488,497,501,548,563,430,446,452,453,454,472,473,484,493,595";
struct ExclusionConfig{std::set<int> ids;std::vector<std::string> names;size_t invalid=0;template<class E>bool hides(const E&e)const{if(ids.count(e.id))return true;for(const auto&name:names)if(sameModelName(e,name))return true;return false;}};
inline void parseModelTokens(const std::string& text,ExclusionConfig& out){size_t start=0;while(start<text.size()){const auto stop=text.find(',',start);const auto token=trimName(text.substr(start,stop==std::string::npos?stop:stop-start));if(!token.empty()){int id=-1;if(decimalModelId(token,id))out.ids.insert(id);else{bool valid=token.size()<=96,letter=false;for(unsigned char c:token){letter=letter||std::isalpha(c);valid=valid&&(c<128)&&(std::isalnum(c)||c=='_'||c=='-'||c==' '||c=='.');}if(valid&&letter){const auto name=lowerName(token);if(std::find(out.names.begin(),out.names.end(),name)==out.names.end())out.names.push_back(name);}else ++out.invalid;}}if(stop==std::string::npos)break;start=stop+1;}}
inline size_t parseModelIds(const std::string& text,std::set<int>& ids){
 size_t invalid=0,start=0;
 while(start<text.size()){
  const auto stop=text.find(',',start);const size_t end=stop==std::string::npos?text.size():stop;
  auto a=text.data()+start,b=text.data()+end;
  while(a<b&&std::isspace(static_cast<unsigned char>(*a)))++a;
  while(b>a&&std::isspace(static_cast<unsigned char>(b[-1])))--b;
  if(a!=b){int id=-1;auto result=std::from_chars(a,b,id);if(result.ec==std::errc{}&&result.ptr==b&&id>=0&&id<1000000)ids.insert(id);else ++invalid;}
  if(stop==std::string::npos)break;start=stop+1;
 }
 return invalid;
}
}
