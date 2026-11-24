#pragma once
#include "StudioUI.hpp"
namespace gt4 {
struct CountryPicker {
    bool open=false,editing=true,dragging=false,scrolling=false;
    double clock=0,openedAt=0,changedAt=0;
    int previous=-1;
    float x=.15f,y=.12f,dragX=0,dragY=0;
    int selected=0,firstRow=0;std::string query,lastQuery;
    static constexpr int columns=4,visibleRows=4;
    float pop(double now,int index) const {
        if(index==selected)return previous==index?1.f:std::clamp(float((now-changedAt)/.06),0.f,1.f);
        if(index==previous)return 1.f-std::clamp(float((now-changedAt)/.06),0.f,1.f);
        return 0.f;
    }
    std::vector<int> matches() const {
        std::vector<int> out;auto needle=countryWide(query);CharLowerBuffW(needle.data(),DWORD(needle.size()));
        for(size_t i=1;i<countryCount;++i)if(countryRegistered[i]) {
            auto value=countryWide(countryLabel(Country(i)));CharLowerBuffW(value.data(),DWORD(value.size()));
            if(needle.empty()||value.find(needle)!=std::wstring::npos)out.push_back(int(i));
        }
        std::sort(out.begin(),out.end(),[](int a,int b){auto x=countryWide(countryLabel(Country(a))),y=countryWide(countryLabel(Country(b)));return CompareStringOrdinal(x.c_str(),int(x.size()),y.c_str(),int(y.size()),TRUE)==CSTR_LESS_THAN;});return out;
    }
    UIRect panel(float w,float h) const {const float s=h/720.f,bw=std::min(w-16*s,700*s),bh=440*s;return {std::clamp(x*w,8*s,std::max(8*s,w-bw-8*s)),std::clamp(y*h,8*s,std::max(8*s,h*.90f-bh)),bw,bh};}
    UIRect grid(float w,float h) const {auto b=panel(w,h);float s=h/720.f;return {b.x+10*s,b.y+62*s,b.w-38*s,328*s};}
    UIRect cell(float w,float h,int index) const {auto g=grid(w,h);float cw=g.w/columns,ch=g.h/visibleRows;return {g.x+(index%columns)*cw,g.y+(index/columns-firstRow)*ch,cw,ch};}
    UIRect scrollTrack(float w,float h) const {auto b=panel(w,h);auto g=grid(w,h);return {b.x+b.w-20*h/720.f,g.y,12*h/720.f,g.h};}
    int maxRow(size_t count) const {return std::max(0,int((count+columns-1)/columns)-visibleRows);}
    UIRect scrollThumb(float w,float h,size_t count) const {auto t=scrollTrack(w,h);const int rows=std::max(visibleRows,int((count+columns-1)/columns));float height=t.h*visibleRows/rows;return {t.x,t.y+(t.h-height)*firstRow/std::max(1,maxRow(count)),t.w,height};}
    void clamp() {const auto list=matches();if(query!=lastQuery){selected=firstRow=0;lastQuery=query;}selected=std::clamp(selected,0,std::max(0,int(list.size())-1));firstRow=std::clamp(firstRow,0,maxRow(list.size()));}
    void move(int dx,int dy) {auto list=matches();if(list.empty())return;editing=false;selected=std::clamp(selected+dx+dy*columns,0,int(list.size())-1);firstRow=std::clamp(firstRow,std::max(0,selected/columns-visibleRows+1),selected/columns);}
};
}
