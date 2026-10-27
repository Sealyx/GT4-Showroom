#pragma once
#include "ModelFilter.hpp"
#include <array>
#include <windows.h>
namespace gt4 {
enum class Country {Unknown,USA,Japan,Italy,France,UK,Germany,Sweden,China,SouthKorea,Canada,Australia,Russia,Denmark,Brazil,Indonesia,Malaysia};
inline constexpr const char* countryKeys[]={"Unknown","USA","Japan","Italy","France","UK","Germany","Sweden","China","SouthKorea","Canada","Australia","Russia","Denmark","Brazil","Indonesia","Malaysia"};
inline constexpr size_t countryCount=sizeof(countryKeys)/sizeof(countryKeys[0]);
inline const char* countryLabel(Country c){return c==Country::SouthKorea?"South Korea":countryKeys[size_t(c)];}
struct CountryDefault {int id;const char* model;Country country;};
#include "CountryDefaults.inc"
inline bool vanillaVehicle(int id){return id>=400&&id<=611;}
template<class E>Country defaultCountry(const E&e){if(vanillaVehicle(e.id))return countryDefaults[size_t(e.id-400)].country;return Country::Unknown;}
struct CountryConfig {
 std::array<ExclusionConfig,countryCount> overrides,vanilla;bool vanillaConfigured=false;size_t invalid=0;
 template<class E>Country resolve(const E&e)const{
  if(vanillaVehicle(e.id)){
   if(!vanillaConfigured)return defaultCountry(e);  
   for(size_t i=1;i<countryCount;++i)if(vanilla[i].ids.count(e.id))return Country(i);
   const std::string original=countryDefaults[size_t(e.id-400)].model;
   for(size_t i=1;i<countryCount;++i)for(const auto&name:vanilla[i].names)if(name==original||sameModelName(e,name))return Country(i);
   return Country::Unknown;  
  }
  for(size_t i=1;i<countryCount;++i)if(overrides[i].ids.count(e.id))return Country(i);
  for(size_t i=1;i<countryCount;++i)for(const auto&name:overrides[i].names)if(sameModelName(e,name))return Country(i);
  return defaultCountry(e);
 }
};
inline CountryConfig readCountries(const std::wstring&ini){CountryConfig result;
 std::array<wchar_t,32768> section{};GetPrivateProfileSectionNamesW(section.data(),DWORD(section.size()),ini.c_str());
 for(const wchar_t*name=section.data();*name;name+=wcslen(name)+1)if(_wcsicmp(name,L"VanillaCountries")==0)result.vanillaConfigured=true;
 for(int group=0;group<2;++group)for(size_t i=1;i<countryCount;++i){const std::string key=countryKeys[i];const std::wstring wkey(key.begin(),key.end());std::array<wchar_t,32768> value{};const DWORD n=GetPrivateProfileStringW(group?L"VanillaCountries":L"Countries",wkey.c_str(),L"",value.data(),DWORD(value.size()),ini.c_str());std::string text;for(DWORD j=0;j<n;++j)text+=value[j]<128?char(value[j]):'?';auto&tokens=(group?result.vanilla:result.overrides)[i];parseModelTokens(text,tokens);result.invalid+=tokens.invalid+(n>=value.size()-1);}return result;}
}
