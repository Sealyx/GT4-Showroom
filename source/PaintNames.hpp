#pragma once
#include "ModelNames.hpp"
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <vector>
namespace gt4 {
inline std::string properPaintName(std::string name){bool start=true;for(char&c:name){if(c>='A'&&c<='Z')c=char(c+32);if(c>='a'&&c<='z'){if(start)c=char(c-32);start=false;}else if(c>='0'&&c<='9')start=false;else if(c!='\'')start=true;}return name;}
struct NamedColor {int id=0;uint32_t rgb=0;std::string name,source;int priority=0;};
struct PaintNames {
 std::vector<NamedColor> rows;
 void parse(std::istream&in,const std::string&source,int priority){
  std::string line;bool palette=false;int ordinal=0;size_t lines=0;
  while(std::getline(in,line)&&++lines<100000){auto text=trimName(line);if(text.empty()||text[0]=='#'||text[0]==';')continue;const auto hash=text.find('#');const auto body=trimName(text.substr(0,hash));const auto section=lowerName(body);if(section=="col"){palette=true;ordinal=0;continue;}if(section=="end"){palette=false;continue;}if(!palette)continue;
   auto values=body;std::replace(values.begin(),values.end(),',',' ');std::istringstream rgb(values);int r,g,b;if(!(rgb>>r>>g>>b)||r<0||r>255||g<0||g>255||b<0||b>255)continue;
   int id=ordinal++;std::string name;if(hash!=std::string::npos){name=trimName(text.substr(hash+1));size_t digits=0;while(digits<name.size()&&name[digits]>='0'&&name[digits]<='9')++digits;if(digits&&digits<name.size()&&(name[digits]==' '||name[digits]=='\t')){int explicitId=0;if(decimalModelId(name.substr(0,digits),explicitId)&&explicitId<65536)id=explicitId;name=trimName(name.substr(digits));}
    const auto tab=name.find('\t'),spaces=name.find("  ");const auto end=std::min(tab,spaces);if(end!=std::string::npos)name=trimName(name.substr(0,end));
   }
   if(!name.empty()&&name.size()<=160&&rows.size()<65536)rows.push_back({id,uint32_t((r<<16)|(g<<8)|b),properPaintName(name),source,priority});
  }
 }
 std::string name(int id,uint32_t rgb)const{
  const NamedColor*best=nullptr;for(const auto&row:rows)if(row.rgb==rgb&&row.id==id&&(!best||row.priority>=best->priority))best=&row;
  if(best)return best->name;
  // Partial Mod Loader palettes can be appended at a different native index.
  // Resolve by RGB only if all equally preferred matches agree on the name.
  bool ambiguous=false;for(const auto&row:rows)if(row.rgb==rgb){if(!best||row.priority>best->priority){best=&row;ambiguous=false;}else if(row.priority==best->priority&&lowerName(row.name)!=lowerName(best->name))ambiguous=true;}
  return best&&!ambiguous?best->name:"Color "+std::to_string(id);
 }
};
inline PaintNames paintNames;
}
