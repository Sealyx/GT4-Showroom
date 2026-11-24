#pragma once
#include "ModelFilter.hpp"
#include <array>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>
#include <cctype>

namespace gt4
// self explanatory, handles countries applied to vehicles
// country is for add-on stuff, vanillacountries is well.. for vanilla
{
    enum class Country
    {
        Unknown,
        USA,
        Japan,
        Italy,
        France,
        UK,
        Germany,
        Sweden,
        China,
        SouthKorea,
        Canada,
        Australia,
        Russia,
        Denmark,
        Brazil,
        Indonesia,
        Malaysia, Netherlands, Belgium, Spain, Serbia, Czech, Romania, Turkey
    };
    inline constexpr size_t countryCount=49, builtInCountryCount=24;
    inline constexpr unsigned countryContextBits=49;
    inline constexpr uint64_t allCountryBits=(uint64_t(1)<<countryCount)-1;
    inline constexpr const char* countryFolderNames[]{"english","indonesian","spanish","chinese","japanese","korean","russian"};
    inline constexpr const char* builtInCountryKeys[]{"Unknown","UnitedStates","Japan","Italy","France","UnitedKingdom","Germany","Sweden","China","SouthKorea","Canada","Australia","Russia","Denmark","Brazil","Indonesia","Malaysia","Netherlands","Belgium","Spain","Serbia","Czech","Romania","Turkey"};
    inline constexpr const char* builtInCountryLabels[]{"Unassigned","United States","Japan","Italy","France","United Kingdom","Germany","Sweden","China","South Korea","Canada","Australia","Russia","Denmark","Brazil","Indonesia","Malaysia","Netherlands","Belgium","Spain","Serbia","Czech Republic","Romania","Turkey"};
    inline std::array<std::string,countryCount> countryCanonical=[] {std::array<std::string,countryCount> v{};for(size_t i=0;i<countryCount;++i)v[i]=i<builtInCountryCount?builtInCountryKeys[i]:"Custom"+std::to_string(i-builtInCountryCount+1);return v;}();
    inline std::array<const char*,countryCount> countryKeys=[] {std::array<const char*,countryCount> v{};for(size_t i=0;i<countryCount;++i)v[i]=countryCanonical[i].c_str();return v;}();
    inline std::array<std::array<std::string,countryCount>,7> countryTranslations{};
    inline std::array<bool,countryCount> countryRegistered=[] {std::array<bool,countryCount> v{};for(size_t i=0;i<builtInCountryCount;++i)v[i]=true;return v;}();
    inline int countryLanguage=0;
    inline std::string countryTrim(std::string value) {const auto a=value.find_first_not_of(" \t\r\n");if(a==std::string::npos)return {};return value.substr(a,value.find_last_not_of(" \t\r\n")-a+1);}
    inline std::wstring countryWide(const std::string& value) {
        const int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value.data(),int(value.size()),nullptr,0);if(n<=0)return {};
        std::wstring out(size_t(n),L'\0');MultiByteToWideChar(CP_UTF8,0,value.data(),int(value.size()),out.data(),n);return out;
    }
    inline std::string countryUtf8(const std::wstring& value) {
        const int n=WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),nullptr,0,nullptr,nullptr);if(n<=0)return {};
        std::string out(size_t(n),'\0');WideCharToMultiByte(CP_UTF8,0,value.data(),int(value.size()),out.data(),n,nullptr,nullptr);return out;
    }
    inline bool countrySame(const std::string& a,const std::string& b) {const auto x=countryWide(a),y=countryWide(b);return CompareStringOrdinal(x.c_str(),int(x.size()),y.c_str(),int(y.size()),TRUE)==CSTR_EQUAL;}
    inline const char* countryLabel(Country c) {
        const auto id=size_t(c);if(id>=countryCount)return "Unassigned";
        const auto& label=countryTranslations[size_t(std::clamp(countryLanguage,0,6))][id];
        return label.empty()?(id<builtInCountryCount?builtInCountryLabels[id]:countryCanonical[id].c_str()):label.c_str();
    }
    inline void loadCountryRegistry(const std::filesystem::path& root) {
        std::array<std::string,7> emptyNames{"Empty","Kosong","Vacío","空","未登録","비어 있음","Пусто"};
        for(size_t language=0;language<7;++language) {
            countryTranslations[language].fill({});std::ifstream input(root/countryFolderNames[language]/"countries.fxt",std::ios::binary);
            std::vector<std::pair<unsigned,std::string>> values;std::string line;
            while(std::getline(input,line)) {
                if(line.compare(0,3,"\xEF\xBB\xBF")==0)line.erase(0,3);
                line=countryTrim(line);if(line.empty()||line[0]=='#'||line[0]==';')continue;
                const auto split=line.find_first_of(" \t");if(split==std::string::npos)continue;
                const auto key=line.substr(0,split),value=countryTrim(line.substr(split+1));
                if(key=="EMPTY_NAME"){emptyNames[language]=value;continue;}
                if(key.rfind("COUNTRY",0)!=0)continue;
                try{size_t used=0;auto id=std::stoul(key.substr(7),&used);if(used==key.size()-7&&id<countryCount)values.emplace_back(unsigned(id),value);}catch(...){}
            }
            for(const auto& item:values)if(!item.second.empty()&&!countrySame(item.second,emptyNames[language])&&!countrySame(item.second,"Empty"))countryTranslations[language][item.first]=item.second;
        }
        for(size_t i=0;i<countryCount;++i) {
            std::string name;for(size_t language=0;language<7&&name.empty();++language)name=countryTranslations[language][i];
            countryRegistered[i]=i<builtInCountryCount||!name.empty();
            if(i<builtInCountryCount)countryCanonical[i]=builtInCountryKeys[i];
            else {countryCanonical[i]=name;countryCanonical[i].erase(std::remove_if(countryCanonical[i].begin(),countryCanonical[i].end(),[](unsigned char c){return std::isspace(c)!=0;}),countryCanonical[i].end());if(countryCanonical[i].empty())countryCanonical[i]="Custom"+std::to_string(i-builtInCountryCount+1);}
            countryKeys[i]=countryCanonical[i].c_str();
            if(countryRegistered[i]&&i>=builtInCountryCount)for(auto& translated:countryTranslations)if(translated[i].empty())translated[i]=name;
        }
    }

    struct CountryDefault
    {
        int id;
        const char* model;
        Country country;
    };

#include "CountryDefaults.inc"

    inline bool vanillaVehicle(int id)
    {
        return id >= 400 && id <= 611;
    }

    template <class E> Country defaultCountry(const E& e)
    {
        if (vanillaVehicle(e.id)) {
            return countryDefaults[size_t(e.id - 400)].country;
        }
        return Country::Unknown;
    }

    struct CountryConfig
    {
        std::array<ExclusionConfig, countryCount> overrides, vanilla;
        bool vanillaConfigured = false;
        size_t invalid = 0;

        template <class E> Country resolve(const E& e) const
        {
            if (vanillaVehicle(e.id)) {
                if (!vanillaConfigured) {
                    return defaultCountry(e);
                }
                for (size_t i = 1; i < countryCount; ++i) {
                    if (countryRegistered[i] && vanilla[i].ids.count(e.id)) {
                        return Country(i);
                    }
                }
                const std::string original = countryDefaults[size_t(e.id - 400)].model;
                for (size_t i = 1; i < countryCount; ++i) {
                    if(!countryRegistered[i])continue;
                    for (const auto& name : vanilla[i].names) {
                        if (name == original || sameModelName(e, name)) {
                            return Country(i);
                        }
                    }
                }
                return Country::Unknown;
            }
            for (size_t i = 1; i < countryCount; ++i) {
                if (countryRegistered[i] && overrides[i].ids.count(e.id)) {
                    return Country(i);
                }
            }
            for (size_t i = 1; i < countryCount; ++i) {
                if(!countryRegistered[i])continue;
                for (const auto& name : overrides[i].names) {
                    if (sameModelName(e, name)) {
                        return Country(i);
                    }
                }
            }
            return defaultCountry(e);
        }
    };

    inline CountryConfig readCountries(const std::wstring& ini)
    {
        CountryConfig result;
        std::array<wchar_t, 32768> section{};
        GetPrivateProfileSectionNamesW(section.data(), DWORD(section.size()), ini.c_str());
        for (const wchar_t* name = section.data(); *name; name += wcslen(name) + 1) {
            if (_wcsicmp(name, L"VanillaCountries") == 0) {
                result.vanillaConfigured = true;
            }
        }
        for (int group = 0; group < 2; ++group) {
            for (size_t i = 1; i < countryCount; ++i) {
                const std::string key = countryKeys[i];
                const std::wstring wkey=countryWide(key);
                std::array<wchar_t, 32768> value{};
                DWORD n = GetPrivateProfileStringW(group ? L"VanillaCountries" : L"Countries",
                                                         wkey.c_str(),
                                                         L"",
                                                         value.data(),
                                                         DWORD(value.size()),
                                                         ini.c_str());
                if(i==size_t(Country::USA)&&!n)n=GetPrivateProfileStringW(group?L"VanillaCountries":L"Countries",L"USA",L"",value.data(),DWORD(value.size()),ini.c_str());
                if(i==size_t(Country::USA)&&!n)n=GetPrivateProfileStringW(group?L"VanillaCountries":L"Countries",L"United States",L"",value.data(),DWORD(value.size()),ini.c_str());
                if(i==size_t(Country::UK)&&!n)n=GetPrivateProfileStringW(group?L"VanillaCountries":L"Countries",L"UK",L"",value.data(),DWORD(value.size()),ini.c_str());
                std::string text;
                for (DWORD j = 0; j < n; ++j) {
                    text += value[j] < 128 ? char(value[j]) : '?';
                }
                auto& tokens = (group ? result.vanilla : result.overrides)[i];
                parseModelTokens(text, tokens);
                result.invalid += tokens.invalid + (n >= value.size() - 1);
            }
        }
        return result;
    }
}
