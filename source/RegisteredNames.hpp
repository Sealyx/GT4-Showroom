#pragma once
#include "ModelNames.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
namespace gt4 {
class RegisteredNames {
 struct Row{uint32_t key=0;std::string name;};std::map<int,Row> rows;
public:
 void clear(){rows.clear();}
 void parse(std::istream&in,bool sectionsOnly=false){std::string line;bool cars=false;while(std::getline(in,line)){
  line=trimName(line.substr(0,line.find_first_of("#;")));const auto folded=lowerName(line);
  if(!folded.empty()&&folded.front()=='['&&folded.back()==']'){cars=folded=="[vehicles.ide]";continue;}
  if(folded=="cars"){if(!sectionsOnly)cars=true;continue;}if(folded=="end"){cars=false;continue;}if(!cars||line.empty())continue;
  std::replace(line.begin(),line.end(),',',' ');std::istringstream fields(line);std::string number,name;int id=0;
  if(!(fields>>number>>name)||!decimalModelId(number,id)||name.empty()||name.size()>96)continue;
  if(!std::all_of(name.begin(),name.end(),[](unsigned char c){return std::isalnum(c)||c=='_'||c=='-';}))continue;
  rows[id]={modelNameKey(name),name};
 }}
 void load(const std::filesystem::path&file,bool sectionsOnly=false){std::ifstream stream(file);if(stream)parse(stream,sectionsOnly);}
 std::string find(int id,uint32_t key)const{const auto it=rows.find(id);return it!=rows.end()&&it->second.key==key?it->second.name:std::string{};}
 size_t size()const{return rows.size();}
};
inline RegisteredNames registeredNames;
}
