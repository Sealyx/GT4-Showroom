#pragma once
#include <windows.h>
#include <filesystem>
#include <set>
#include <vector>
#include <cwchar>
namespace gt4
{
    inline std::set<int> readFavoriteIds(const std::filesystem::path& path)
    {
        std::vector<wchar_t> section(1048576);
        const DWORD size=GetPrivateProfileSectionW(L"Favorites",section.data(),DWORD(section.size()),path.c_str());
        std::set<int> ids;
        if(size>=section.size()-2)return ids;
        for(const wchar_t* line=section.data();*line;line+=wcslen(line)+1){
            wchar_t* end=nullptr;const long id=std::wcstol(line,&end,10);
            if(end==line||*end!=L'='||id<0)continue;
            wchar_t* valueEnd=nullptr;const long selected=std::wcstol(end+1,&valueEnd,10);
            if(valueEnd!=end+1&&*valueEnd==0&&selected)ids.insert(int(id));
        }
        return ids;
    }

    inline bool saveFavoriteIds(const std::filesystem::path& path,const std::set<int>& ids)
    {
        if(ids.empty()){
            if(GetFileAttributesW(path.c_str())==INVALID_FILE_ATTRIBUTES)return true;
            std::vector<wchar_t> sections(65536);
            const DWORD size=GetPrivateProfileSectionNamesW(sections.data(),DWORD(sections.size()),path.c_str());
            if(size>=sections.size()-2)return false;
            bool other=false;
            for(const wchar_t* section=sections.data();*section;section+=wcslen(section)+1)
                if(_wcsicmp(section,L"Favorites")!=0)other=true;
            if (!WritePrivateProfileStringW(L"Favorites",nullptr,nullptr,path.c_str())) return false;
            return other || DeleteFileW(path.c_str())!=FALSE;
        }
        std::error_code error;std::filesystem::create_directories(path.parent_path(),error);if(error)return false;
        std::vector<wchar_t> values;
        for(int id:ids){
            if(id<0)return false;
            const auto value=std::to_wstring(id)+L"=1";
            values.insert(values.end(),value.begin(),value.end());values.push_back(0);
        }
        values.push_back(0);
        return WritePrivateProfileSectionW(L"Favorites",values.data(),path.c_str())!=FALSE;
    }

    template<class Entries> bool saveFavorites(const std::filesystem::path& path,const Entries& entries)
    {
        auto ids=readFavoriteIds(path);
        for(const auto& entry:entries){if(entry.favorite)ids.insert(entry.id);else ids.erase(entry.id);}
        return saveFavoriteIds(path,ids);
    }
}
