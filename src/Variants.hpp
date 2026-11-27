#pragma once
#include "Showroom.hpp"
#include "ModelFilter.hpp"
#include <filesystem>
#include <sstream>
#include <windows.h>
namespace gt4 {
inline bool variantMatches(const ExclusionConfig& token,const Entry& e) {
 if(token.ids.count(e.id))return true;
 for(const auto& name:token.names)if((e.modelKey && e.modelKey==modelNameKey(name)) || lowerName(e.modelName)==name)return true;
 return false;
}
struct VariantRule { ExclusionConfig base; std::vector<ExclusionConfig> choices; };
struct VariantConfig {
 std::vector<VariantRule> rules;
 bool hidden(const Entry& e)const {for(const auto& r:rules)for(const auto& c:r.choices)if(variantMatches(c,e))return true;return false;}
 const VariantRule* find(const Entry& e)const {for(const auto& r:rules)if(variantMatches(r.base,e))return &r;return nullptr;}
 std::vector<Entry> choicesFor(const Entry& base,const std::vector<Entry>& catalog,const std::set<int>& excluded)const {
  std::vector<Entry> result{base};
  if(const auto* rule=find(base))for(const auto& token:rule->choices)for(const auto& e:catalog){
   if(!variantMatches(token,e) || excluded.count(e.id) || std::any_of(result.begin(),result.end(),[&](const Entry& old){return old.id==e.id;}))continue;
   auto variant=e;if(variant.classId<0){variant.classId=base.classId;variant.category=base.category;}
   result.push_back(std::move(variant));
  }
  return result;
 }
 void load(const std::filesystem::path& path){rules.clear();std::vector<wchar_t> sections(65536);GetPrivateProfileSectionNamesW(sections.data(),DWORD(sections.size()),path.c_str());
 for(const wchar_t* section=sections.data();*section;section+=wcslen(section)+1){VariantRule r;std::wstring ws(section);parseModelTokens(std::string(ws.begin(),ws.end()),r.base);if(r.base.invalid||(r.base.ids.empty()&&r.base.names.empty()))continue;
 std::vector<wchar_t> values(65536);GetPrivateProfileStringW(section,L"Variant",L"",values.data(),DWORD(values.size()),path.c_str());
 std::wistringstream input(values.data());std::wstring value;
 while(std::getline(input,value,L',')){ExclusionConfig c;parseModelTokens(std::string(value.begin(),value.end()),c);if(!c.invalid&&c.ids.size()+c.names.size()==1)r.choices.push_back(std::move(c));}
 if(!r.choices.empty())rules.push_back(std::move(r));}
 }
};
}
