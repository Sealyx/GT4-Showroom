#pragma once
#include "Countries.hpp"
#include <map>
namespace gt4 {
struct CountryIniOrder {bool operator()(const std::wstring&a,const std::wstring&b) const {return CompareStringOrdinal(a.c_str(),int(a.size()),b.c_str(),int(b.size()),TRUE)==CSTR_LESS_THAN;}};
inline bool updateCountryIni(const std::wstring& ini,int assigned=-1,const std::string& model={},bool vanilla=false) {
    if(assigned>=0&&(assigned==0||assigned>=int(countryCount)||!countryRegistered[size_t(assigned)]))return false;
    for(const auto section:{L"Countries",L"VanillaCountries"}) {
        std::vector<wchar_t> buffer(65536);const DWORD length=GetPrivateProfileSectionW(section,buffer.data(),DWORD(buffer.size()),ini.c_str());if(length>=buffer.size()-2)return false;
        std::map<std::wstring,std::wstring,CountryIniOrder> values;
        for(const wchar_t* line=buffer.data();*line;line+=wcslen(line)+1){const std::wstring item(line);const auto split=item.find(L'=');if(split!=std::wstring::npos)values[item.substr(0,split)]=item.substr(split+1);}
        for(const auto& alias:std::array<std::pair<const wchar_t*,const wchar_t*>,3>{{{L"USA",L"UnitedStates"},{L"United States",L"UnitedStates"},{L"UK",L"UnitedKingdom"}}}) {
            const auto old=values.find(alias.first);
            if(old==values.end())continue;
            if(!values.count(alias.second)||values[alias.second].empty())values[alias.second]=old->second;
            else if(!old->second.empty()&&values[alias.second]!=old->second)values[alias.second]+=L","+old->second;
            values.erase(old);
        }
        for(size_t i=1;i<countryCount;++i)if(countryRegistered[i])values.try_emplace(countryWide(countryKeys[i]),L"");
        values.erase(L"Empty");values.erase(L"Kosong");
        if(assigned>0&&vanilla==(std::wstring(section)==L"VanillaCountries")) {
            auto& value=values[countryWide(countryKeys[size_t(assigned)])];const auto token=countryWide(model);
            std::wistringstream input(value);std::wstring part;bool exists=false;while(std::getline(input,part,L','))if(countrySame(countryUtf8(part),model))exists=true;
            if(!exists){if(!value.empty())value+=L",";value+=token;}
        }
        std::wstring output;for(const auto& item:values){output+=item.first+L"="+item.second;output.push_back(L'\0');}output.push_back(L'\0');
        if(std::wstring(buffer.data(),length)==std::wstring(output.data(),output.size()-1))continue;
        if(!WritePrivateProfileSectionW(section,output.c_str(),ini.c_str()))return false;
    }
    return true;
}
}
