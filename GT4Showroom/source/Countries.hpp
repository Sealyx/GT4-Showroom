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
 std::array<ExclusionConfig,countryCount> overrides;size_t invalid=0;
 template<class E>Country resolve(const E&e)const{
  // Vanilla origin cannot be cleared or reassigned through this add-on section.
  if(vanillaVehicle(e.id))return defaultCountry(e);
  for(size_t i=1;i<countryCount;++i)if(overrides[i].ids.count(e.id))return Country(i);
  for(size_t i=1;i<countryCount;++i)for(const auto&name:overrides[i].names)if(sameModelName(e,name))return Country(i);
  return defaultCountry(e);
 }
};
inline CountryConfig readCountries(const std::wstring&ini){CountryConfig result;for(size_t i=1;i<countryCount;++i){const std::string key=countryKeys[i];const std::wstring wkey(key.begin(),key.end());std::array<wchar_t,32768> value{};const DWORD n=GetPrivateProfileStringW(L"Countries",wkey.c_str(),L"",value.data(),DWORD(value.size()),ini.c_str());std::string text;for(DWORD j=0;j<n;++j)text+=value[j]<128?char(value[j]):'?';parseModelTokens(text,result.overrides[i]);result.invalid+=result.overrides[i].invalid+(n>=value.size()-1);}return result;}
}
