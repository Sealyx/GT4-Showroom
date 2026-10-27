#pragma once
#include <string>
#include <cctype>
#include <cstdint>
#include <charconv>
#include <vector>
namespace gt4 {
inline std::string trimName(const std::string& s){const auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");return a==std::string::npos?std::string{}:s.substr(a,b-a+1);}
inline std::string lowerName(std::string s){for(auto& c:s)if(c>='A'&&c<='Z')c=char(c+32);return s;}
inline uint32_t modelNameKey(const std::string& name){uint32_t key=0xFFFFFFFF;for(unsigned char c:name){if(c>='a'&&c<='z')c=uint8_t(c-32);key^=c;for(int bit=0;bit<8;++bit)key=(key>>1)^((key&1)?0xEDB88320u:0);}return key;}
inline bool decimalModelId(const std::string& text,int& id){if(text.empty())return false;for(char c:text)if(c<'0'||c>'9')return false;const auto r=std::from_chars(text.data(),text.data()+text.size(),id);return r.ec==std::errc{}&&r.ptr==text.data()+text.size()&&id>=0&&id<1000000;}
template<class E>bool sameModelName(const E&e,const std::string& name){return (e.modelKey&&e.modelKey==modelNameKey(name))||lowerName(e.name)==lowerName(name);}
template<class E>size_t findModel(const std::vector<E>& entries,const std::string& raw){const auto query=trimName(raw);if(query.empty())return entries.size();int id=-1;const bool numeric=decimalModelId(query,id);bool onlyDigits=true;for(char c:query)onlyDigits=onlyDigits&&c>='0'&&c<='9';if(numeric||onlyDigits){for(size_t i=0;i<entries.size();++i)if(numeric&&entries[i].id==id)return i;return entries.size();}for(size_t i=0;i<entries.size();++i)if(sameModelName(entries[i],query))return i;const auto text=lowerName(query);for(size_t i=0;i<entries.size();++i)if(lowerName(entries[i].name).rfind(text,0)==0)return i;for(size_t i=0;i<entries.size();++i)if(lowerName(entries[i].name).find(text)!=std::string::npos)return i;return entries.size();}
}
