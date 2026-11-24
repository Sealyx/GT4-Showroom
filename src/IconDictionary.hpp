#pragma once
#include "VehicleLogos.hpp"
#include <fstream>
#include <new>

namespace gt4 {
class IconDictionary {
    std::vector<uint8_t> bytes;
    std::filesystem::path loaded;
    std::map<std::string, size_t> entries;
    static uint32_t u32(const uint8_t* p) {
        uint32_t value; std::memcpy(&value,p,4); return value;
    }
    static unsigned u16(const uint8_t* p) {
        uint16_t value; std::memcpy(&value,p,2); return value;
    }
    void open(const std::filesystem::path& file) {
        if(loaded==file) return;
        loaded=file; bytes.clear(); entries.clear();
        std::error_code ec;
        const auto length=std::filesystem::file_size(file,ec);
        if(ec || length<28 || length>256*1024*1024) return;
        try {bytes.resize(size_t(length));}catch(const std::bad_alloc&){return;}
        std::ifstream stream(file,std::ios::binary);
        if(!stream.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size())) ||
           u32(bytes.data())!=0x16 || u32(bytes.data()+4)!=bytes.size()-12) {
            bytes.clear(); return;
        }
        for(size_t at=12;at+12<=bytes.size();) {
            const auto* chunk=bytes.data()+at;
            const size_t size=u32(chunk+4),end=at+12+size;
            if(end>bytes.size()) { entries.clear(); return; }
            if(u32(chunk)==0x15 && size>=104) {
                const auto* header=chunk+24;
                if(u32(chunk+12)==1 && u32(chunk+16)>=92 &&
                   u32(chunk+16)<=size-12 && u32(header)==9 &&
                   u32(header+72)==0x500 && u32(header+76)==21 &&
                   header[84]==32 && header[85]==1 && header[87]==1) {
                    const unsigned w=u16(header+80),h=u16(header+82);
                    const size_t pixels=size_t(w)*h*4;
                    if(w && h && w<=4096 && h<=4096 && pixels==u32(header+88) &&
                       pixels<=u32(chunk+16)-92 && std::memchr(header+8,0,32))
                        entries.emplace(reinterpret_cast<const char*>(header+8),at+24);
                }
            }
            at=end;
        }
    }
public:
    void reset(){bytes.clear();entries.clear();loaded.clear();}
    bool pixels(const std::filesystem::path& file,const std::string& name,
                unsigned& width,unsigned& height,std::vector<uint32_t>& output) {
        open(file);const auto found=entries.find(name);
        if(found==entries.end())return false;
        const auto* header=bytes.data()+found->second;
        width=u16(header+80);height=u16(header+82);
        try {output.resize(size_t(width)*height);}catch(const std::bad_alloc&){return false;}
        std::memcpy(output.data(),header+92,output.size()*4);return true;
    }
    VehicleLogos::Image load(IDirect3DDevice9* device,const std::filesystem::path& file,
                             const std::wstring& name) {
        open(file);
        const std::string key(name.begin(),name.end());
        const auto found=entries.find(key);
        VehicleLogos::Image image;
        if(!device || found==entries.end()) return image;
        const auto* header=bytes.data()+found->second;
        const unsigned w=u16(header+80),h=u16(header+82);
        if(FAILED(device->CreateTexture(w,h,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,
                                       &image.texture,nullptr))) return image;
        D3DLOCKED_RECT lock{};
        if(FAILED(image.texture->LockRect(0,&lock,nullptr,0))) {
            image.texture->Release(); return {};
        }
        for(unsigned y=0;y<h;++y)
            std::memcpy(static_cast<uint8_t*>(lock.pBits)+y*lock.Pitch,header+92+size_t(y)*w*4,w*4);
        image.texture->UnlockRect(0); image.width=w; image.height=h;
        return image;
    }
};
}
