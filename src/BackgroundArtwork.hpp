#pragma once
#include "BackgroundArtworkData.hpp"
#include "VehicleLogos.hpp"
#include <fstream>
#include <map>

namespace gt4 {
class BackgroundArtwork {
    struct Record { unsigned w=0,h=0;D3DFORMAT format=D3DFMT_UNKNOWN;uint32_t length=0;uint64_t offset=0; };
    struct Cached { IDirect3DTexture9* texture=nullptr;bool checked=false,original=false;uint64_t used=0;size_t bytes=0; };
    std::filesystem::path path;
    bool indexed=false;
    std::map<std::string,Record> records;
    std::array<Cached,16> cache{};
    static uint32_t u32(const uint8_t* p) {uint32_t n;std::memcpy(&n,p,4);return n;}
    static unsigned u16(const uint8_t* p) {uint16_t n;std::memcpy(&n,p,2);return n;}
    static uint32_t checksum(const std::vector<uint8_t>& pixels) {
        static const auto table=[] {std::array<uint32_t,256> values{};
            for(unsigned i=0;i<256;++i) {uint32_t c=i;for(int bit=0;bit<8;++bit)c=(c>>1)^((c&1)?0xedb88320u:0);values[i]=c;}
            return values;}();
        uint32_t c=~0u;for(uint8_t b:pixels)c=table[(c^b)&255]^(c>>8);return ~c;
    }
    void index() {
        if(indexed)return;indexed=true;
        std::error_code error;const uint64_t bytes=std::filesystem::file_size(path,error);
        if(error || bytes<28 || bytes>uint64_t(2)*1024*1024*1024)return;
        std::ifstream file(path,std::ios::binary);uint8_t root[12]{};
        if(!file.read(reinterpret_cast<char*>(root),12) || u32(root)!=0x16 || u32(root+4)!=bytes-12)return;
        for(uint64_t offset=12;offset+12<=bytes;) {
            uint8_t chunk[12]{};file.seekg(std::streamoff(offset));
            if(!file.read(reinterpret_cast<char*>(chunk),12))return;
            const uint64_t end=offset+12+u32(chunk+4);if(end>bytes)return;
            if(u32(chunk)==0x15 && end-offset>=116) {
                uint8_t header[104]{};
                if(!file.read(reinterpret_cast<char*>(header),104))return;
                const auto* native=header+12;
                if(u32(header)==1 && u32(header+4)>=92 && u32(header+4)<=end-offset-24 &&
                   u32(native)==9 && native[85]>0 && !(native[87]&2) && std::memchr(native+8,0,32)) {
                    Record record;record.w=u16(native+80);record.h=u16(native+82);
                    record.format=D3DFORMAT(u32(native+76));record.length=u32(native+88);record.offset=offset+116;
                    const bool argb=record.format==D3DFMT_A8R8G8B8 || record.format==D3DFMT_X8R8G8B8;
                    const bool dxt=record.format==D3DFMT_DXT1 || record.format==D3DFMT_DXT3 || record.format==D3DFMT_DXT5;
                    const size_t expected=argb?size_t(record.w)*record.h*4:
                        size_t((record.w+3)/4)*((record.h+3)/4)*(record.format==D3DFMT_DXT1?8:16);
                    if(record.w && record.h && record.w<=4096 && record.h<=4096 &&
                       (argb || dxt) && record.length==expected && record.length<=u32(header+4)-92)
                        records.emplace(reinterpret_cast<const char*>(native+8),record);
                }
            }
            offset=end;
        }
    }
    IDirect3DTexture9* texture(IDirect3DDevice9* device,int preset,uint64_t frame) {
        auto& slot=cache[size_t(preset)];slot.used=frame;
        if(slot.checked)return slot.texture;
        slot.checked=true;index();
        const auto found=records.find(backgroundArtworkNames[size_t(preset)]);if(found==records.end())return nullptr;
        const auto& record=found->second;
        std::vector<uint8_t> pixels;
        try {pixels.resize(record.length);}catch(const std::bad_alloc&){return nullptr;}
        std::ifstream file(path,std::ios::binary);file.seekg(std::streamoff(record.offset));
        if(!file.read(reinterpret_cast<char*>(pixels.data()),std::streamsize(pixels.size())))return nullptr;
        if(record.format==D3DFMT_A8R8G8B8 && record.w==3840 && record.h==2160 &&
           checksum(pixels)==backgroundArtworkChecksums[size_t(preset)]){slot.original=true;return nullptr;}
        size_t total=record.length;
        for(const auto& other:cache)if(other.texture)total+=other.bytes;
        while(total>128*1024*1024) {
            Cached* oldest=nullptr;
            for(auto& other:cache)if(other.texture && other.used!=frame && (!oldest || other.used<oldest->used))oldest=&other;
            if(!oldest){slot.checked=false;return nullptr;}
            total-=oldest->bytes;oldest->texture->Release();*oldest={};
        }
        const bool compressed=record.format==D3DFMT_DXT1 || record.format==D3DFMT_DXT3 || record.format==D3DFMT_DXT5;
        bool mipmaps=false;
        if(!compressed)mipmaps=SUCCEEDED(device->CreateTexture(record.w,record.h,0,D3DUSAGE_AUTOGENMIPMAP,
            record.format,D3DPOOL_MANAGED,&slot.texture,nullptr));
        if(!slot.texture && FAILED(device->CreateTexture(record.w,record.h,1,0,record.format,D3DPOOL_MANAGED,&slot.texture,nullptr)))return nullptr;
        D3DLOCKED_RECT lock{};
        if(FAILED(slot.texture->LockRect(0,&lock,nullptr,0))) {slot.texture->Release();slot.texture=nullptr;return nullptr;}
        const unsigned rows=compressed?(record.h+3)/4:record.h;
        const size_t stride=record.length/rows;
        for(unsigned y=0;y<rows;++y)std::memcpy(static_cast<uint8_t*>(lock.pBits)+y*lock.Pitch,pixels.data()+y*stride,stride);
        slot.texture->UnlockRect(0);slot.bytes=record.length;
        if(mipmaps){slot.texture->SetAutoGenFilterType(D3DTEXF_LINEAR);slot.texture->GenerateMipSubLevels();}
        return slot.texture;
    }
public:
#ifdef BODY_REVIEW
    int status(int preset) const {const auto& slot=cache.at(size_t(preset));return !slot.checked?0:slot.original?1:slot.texture?2:3;}
#endif
    ~BackgroundArtwork(){reset();}
    void reset() {for(auto& slot:cache){if(slot.texture)slot.texture->Release();slot={};}records.clear();indexed=false;}
    void setPath(const std::filesystem::path& next){if(path!=next){reset();path=next;}}
    bool draw(IDirect3DDevice9* device,float w,float h,int preset,float ground,float opacity,uint64_t frame) {
        if(preset<2 || preset>=16 || opacity<=0)return false;
        auto* image=texture(device,preset,frame);if(!image)return false;
        IDirect3DStateBlock9* state=nullptr;
        if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state)) || !state)return false;
        struct Vertex {float x,y,z,rhw;DWORD color;float u,v;};
        const float seam=std::clamp(ground,.05f,.95f);
        const DWORD tint=(DWORD(std::clamp(opacity,0.f,1.f)*255+.5f)<<24)|0xffffff;
        const Vertex vertices[]={{-.5f,-.5f,1,1,tint,0,0},{w-.5f,-.5f,1,1,tint,1,0},
            {-.5f,h*seam-.5f,1,1,tint,0,.56f},{w-.5f,h*seam-.5f,1,1,tint,1,.56f},
            {-.5f,h-.5f,1,1,tint,0,1},{w-.5f,h-.5f,1,1,tint,1,1}};
        device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);
        device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
        device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);
        device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);
        device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);
        device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
        device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
        device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);device->SetTexture(0,image);
        device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_COLORARG2,D3DTA_DIFFUSE);
        device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);device->SetTextureStageState(0,D3DTSS_ALPHAARG2,D3DTA_DIFFUSE);
        device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_MODULATE);device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_MODULATE);
        device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
        device->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_DISABLE);
        device->SetTextureStageState(0,D3DTSS_TEXCOORDINDEX,0);
        device->SetTextureStageState(0,D3DTSS_TEXTURETRANSFORMFLAGS,D3DTTFF_DISABLE);
        device->SetSamplerState(0,D3DSAMP_SRGBTEXTURE,FALSE);
        for(auto sampler:{D3DSAMP_ADDRESSU,D3DSAMP_ADDRESSV})device->SetSamplerState(0,sampler,D3DTADDRESS_CLAMP);
        for(auto sampler:{D3DSAMP_MINFILTER,D3DSAMP_MAGFILTER,D3DSAMP_MIPFILTER})device->SetSamplerState(0,sampler,D3DTEXF_LINEAR);
        device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1);
        const bool drawn=SUCCEEDED(device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,4,vertices,sizeof(Vertex)));
        const bool restored=SUCCEEDED(state->Apply());state->Release();return drawn&&restored;
    }
};
}
