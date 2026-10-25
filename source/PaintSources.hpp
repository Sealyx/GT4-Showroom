#pragma once
#include "PaintNames.hpp"
#include "RegisteredNames.hpp"
#include <filesystem>
#include <map>
#include <set>
#include <windows.h>
namespace gt4 {
inline bool wildMatch(std::string pattern,std::string text){pattern=lowerName(pattern);text=lowerName(text);for(auto&c:pattern)if(c=='\\')c='/';for(auto&c:text)if(c=='\\')c='/';size_t p=0,t=0,star=std::string::npos,retry=0;while(t<text.size()){if(p<pattern.size()&&(pattern[p]=='?'||pattern[p]==text[t])){++p;++t;}else if(p<pattern.size()&&pattern[p]=='*'){star=p++;retry=t;}else if(star!=std::string::npos){p=star+1;t=++retry;}else return false;}while(p<pattern.size()&&pattern[p]=='*')++p;return p==pattern.size();}
struct IniRows {
 std::map<std::string,std::vector<std::pair<std::string,std::string>>> sections;
 void load(const std::filesystem::path&path){std::ifstream in(path);std::string line,section;while(std::getline(in,line)){auto s=trimName(line);if(s.empty()||s[0]==';'||s[0]=='#')continue;if(s[0]=='['&&s.back()==']'){section=lowerName(s.substr(1,s.size()-2));continue;}const auto end=s.find_first_of(";#");s=trimName(s.substr(0,end));const auto equal=s.find('=');sections[section].push_back({lowerName(trimName(s.substr(0,equal))),equal==std::string::npos?"":trimName(s.substr(equal+1))});}}
 std::string value(const std::string&section,const std::string&key,const std::string&fallback="")const{auto it=sections.find(lowerName(section));if(it!=sections.end())for(auto r=it->second.rbegin();r!=it->second.rend();++r)if(r->first==lowerName(key))return r->second;return fallback;}
 std::vector<std::string> list(const std::string&section)const{std::vector<std::string> out;auto it=sections.find(lowerName(section));if(it!=sections.end())for(const auto&row:it->second)out.push_back(row.first);return out;}
};
struct ColorProfile {
 bool ignoreAll=false,excludeAll=false;std::vector<std::string> ignores,includes,ignoreFiles,exclusive;std::map<std::string,int> priority;std::set<std::string> visited;
 static bool truth(const std::string&s){return lowerName(s)=="true"||s=="1";}
 void apply(const IniRows&ini,const std::string&raw,const std::string&current,int depth=0){const auto name=lowerName(raw);if(depth>16||!visited.insert(name).second)return;const auto base="profiles."+name+".";const auto parents=ini.value(base+"config","parents","$None");if(lowerName(parents).find("$none")==std::string::npos){std::istringstream list(parents);std::string parent;while(std::getline(list,parent,',')){parent=trimName(parent);if(lowerName(parent)=="$current")parent=current;if(!parent.empty())apply(ini,parent,current,depth+1);}}
  const auto ignore=ini.value(base+"config","ignoreallmods",ini.value(base+"config","ignoreallfiles",""));if(!ignore.empty())ignoreAll=truth(ignore);const auto exclude=ini.value(base+"config","excludeallmods","");if(!exclude.empty())excludeAll=truth(exclude);
  auto append=[&](const char*key,std::vector<std::string>&to){const auto from=ini.list(base+key);to.insert(to.end(),from.begin(),from.end());};append("ignoremods",ignores);append("includemods",includes);append("ignorefiles",ignoreFiles);append("exclusivemods",exclusive);
  auto it=ini.sections.find(base+"priority");if(it!=ini.sections.end())for(const auto&row:it->second){int n=0;const auto r=std::from_chars(row.second.data(),row.second.data()+row.second.size(),n);if(r.ec==std::errc{})priority[row.first]=std::clamp(n,0,100000);}
 }
 static bool matches(const std::vector<std::string>&patterns,const std::string&name){for(const auto&p:patterns)if(wildMatch(p,name))return true;return false;}
 bool accepts(const std::string&mod,const std::string&file,const std::vector<std::string>&otherExclusive)const{return !ignoreAll&&!matches(ignores,mod)&&(!excludeAll||matches(includes,mod))&&(!matches(otherExclusive,mod)||matches(exclusive,mod))&&!matches(ignoreFiles,file)&&rank(mod)>0;}
 int rank(const std::string&mod)const{auto it=priority.find(lowerName(mod));return it==priority.end()?50:it->second;}
};
struct PaintSourceReport {size_t files=0,rows=0;std::string profile;};
inline PaintSourceReport reloadPaintNames(const std::filesystem::path&gameRoot){
 namespace fs=std::filesystem;paintNames.rows.clear();PaintSourceReport report;auto read=[&](const fs::path&file,int priority){std::ifstream stream(file);if(stream){paintNames.parse(stream,file.u8string(),priority);++report.files;}};
 registeredNames.clear();registeredNames.load(gameRoot/"data/vehicles.ide");
 {std::ifstream dat(gameRoot/"data/gta.dat");std::string row;while(std::getline(dat,row)){row=trimName(row.substr(0,row.find('#')));if(row.size()<4||lowerName(row.substr(0,4))!="ide ")continue;auto file=trimName(row.substr(4));std::replace(file.begin(),file.end(),'\\','/');const fs::path relative(file);if(!relative.is_absolute()&&file.find("..") == std::string::npos)registeredNames.load(gameRoot/relative);}}
 read(gameRoot/"data/carcols.dat",0);const auto mods=gameRoot/"modloader";IniRows ini;ini.load(mods/"modloader.ini");std::error_code ec;
 if(fs::is_directory(mods/".profiles",ec)){std::vector<fs::path> profiles;for(fs::directory_iterator it(mods/".profiles",fs::directory_options::skip_permission_denied,ec),end;it!=end;it.increment(ec)){if(ec){ec.clear();continue;}if(lowerName(it->path().extension().u8string())==".ini")profiles.push_back(it->path());}std::sort(profiles.begin(),profiles.end());for(const auto&file:profiles)ini.load(file);}
 auto current=ini.value("folder.config","profile","Default");std::string chosen=current;
 // Respect explicit -modprof selection and conditional profiles already loaded.
 std::wstring command=GetCommandLineW();std::string cmd(command.begin(),command.end());std::istringstream args(cmd);std::string arg;while(args>>arg)if(lowerName(arg)=="-modprof"){args>>std::ws;if(args.peek()=='\"'){args.get();std::getline(args,chosen,'\"');}else args>>chosen;}
 for(const auto&section:ini.sections)if(section.first.rfind("profiles.",0)==0&&section.first.size()>16&&section.first.substr(section.first.size()-7)==".config"){const auto module=ini.value(section.first,"useifmodule");if(!module.empty()){const auto lib=module.find('.')==std::string::npos?module+".dll":module;if(GetModuleHandleA(lib.c_str()))chosen=section.first.substr(9,section.first.size()-16);}}
 ColorProfile profile;profile.apply(ini,chosen,current);report.profile=chosen;std::vector<std::string> otherExclusive;for(const auto&section:ini.sections){const auto suffix=section.first.find(".exclusivemods");if(section.first.rfind("profiles.",0)==0&&suffix!=std::string::npos&&!profile.visited.count(section.first.substr(9,suffix-9))){const auto values=ini.list(section.first);otherExclusive.insert(otherExclusive.end(),values.begin(),values.end());}}
 struct Source{fs::path file;int rank;};std::vector<Source> sources;
 if(fs::is_directory(mods,ec))for(fs::recursive_directory_iterator it(mods,fs::directory_options::skip_permission_denied,ec),end;it!=end;it.increment(ec)){
  if(ec){ec.clear();continue;}const auto filename=it->path().filename().u8string();if(it->is_symlink(ec)){if(it->is_directory(ec))it.disable_recursion_pending();continue;}if(it->is_directory(ec)){if(!filename.empty()&&(filename[0]=='.'||filename[0]=='_'))it.disable_recursion_pending();continue;}if(lowerName(filename)!="carcols.dat"&&lowerName(it->path().extension().u8string())!=".ide")continue;
  const auto relative=it->path().lexically_relative(mods);auto part=relative.begin();if(part==relative.end())continue;const std::string mod=part->u8string();++part;fs::path inside;for(;part!=relative.end();++part)inside/=*part;if(profile.accepts(mod,inside.generic_u8string(),otherExclusive))sources.push_back({it->path(),profile.rank(mod)});
 }
 std::sort(sources.begin(),sources.end(),[](const Source&a,const Source&b){return a.rank!=b.rank?a.rank<b.rank:lowerName(a.file.u8string())<lowerName(b.file.u8string());});for(const auto&source:sources){if(lowerName(source.file.filename().u8string())=="carcols.dat")read(source.file,source.rank);else registeredNames.load(source.file);}report.rows=paintNames.rows.size();return report;
}
}
