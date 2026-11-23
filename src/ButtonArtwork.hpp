#pragma once
#include "ButtonArtworkData.hpp"
#include "IconDictionary.hpp"

namespace gt4 {
class ButtonArtwork {
    struct Cached { VehicleLogos::Image image;bool checked=false,original=false; };
    std::filesystem::path path;
    IconDictionary dictionary;
    std::map<std::string,Cached> cache;
    static uint32_t checksum(const std::vector<uint32_t>& pixels) {
        static const auto table=[] {std::array<uint32_t,256> values{};
            for(unsigned i=0;i<256;++i){uint32_t c=i;for(int b=0;b<8;++b)c=(c>>1)^((c&1)?0xedb88320u:0);values[i]=c;}return values;}();
        uint32_t c=~0u;const auto* bytes=reinterpret_cast<const uint8_t*>(pixels.data());
        for(size_t i=0;i<pixels.size()*4;++i)c=table[(c^bytes[i])&255]^(c>>8);return ~c;
    }
public:
#ifdef BODY_REVIEW
    int status(const std::string& name) const {const auto& slot=cache.at(name);return !slot.checked?0:slot.original?1:slot.image.texture?2:3;}
#endif
    ~ButtonArtwork(){reset();}
    void reset(){for(auto& item:cache)if(item.second.image.texture)item.second.image.texture->Release();cache.clear();dictionary.reset();}
    void setPath(const std::filesystem::path& next){if(next!=path){reset();path=next;}}
    const VehicleLogos::Image& get(IDirect3DDevice9* device,const char* name) {
        auto& slot=cache[name];if(slot.checked)return slot.image;slot.checked=true;
        const auto found=std::find(buttonArtworkNames.begin(),buttonArtworkNames.end(),std::string(name));
        if(found==buttonArtworkNames.end())return slot.image;
        unsigned w=0,h=0;std::vector<uint32_t> pixels;
        if(!dictionary.pixels(path,name,w,h,pixels) || pixels.size()>4194304)return slot.image;
        const size_t index=size_t(found-buttonArtworkNames.begin());
        if(w==buttonArtworkWidths[index] && h==buttonArtworkHeights[index] && checksum(pixels)==buttonArtworkChecksums[index]){slot.original=true;return slot.image;}
        bool mips=SUCCEEDED(device->CreateTexture(w,h,0,D3DUSAGE_AUTOGENMIPMAP,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&slot.image.texture,nullptr));
        if(!slot.image.texture && FAILED(device->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&slot.image.texture,nullptr)))return slot.image;
        D3DLOCKED_RECT lock{};if(FAILED(slot.image.texture->LockRect(0,&lock,nullptr,0))){slot.image.texture->Release();slot.image={};return slot.image;}
        for(unsigned y=0;y<h;++y){auto* row=static_cast<uint8_t*>(lock.pBits)+y*lock.Pitch;
            for(unsigned x=0;x<w;++x){const auto pixel=pixels[size_t(y)*w+x];const unsigned a=pixel>>24;
                row[4*x]=uint8_t(((pixel&255)*a+127)/255);row[4*x+1]=uint8_t((((pixel>>8)&255)*a+127)/255);
                row[4*x+2]=uint8_t((((pixel>>16)&255)*a+127)/255);row[4*x+3]=uint8_t(a);}}
        slot.image.texture->UnlockRect(0);slot.image.width=w;slot.image.height=h;
        if(mips){slot.image.texture->SetAutoGenFilterType(D3DTEXF_LINEAR);slot.image.texture->GenerateMipSubLevels();}
        return slot.image;
    }
};
}
