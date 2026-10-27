#pragma once
#include "ModelNames.hpp"
#include <windows.h>
#include <wincodec.h>
#include <d3d9.h>
#include <filesystem>
#include <map>
#include <cstring>
#include <vector>
namespace gt4 {
 
 
class VehicleLogos {
 struct Image{IDirect3DTexture9* texture=nullptr;unsigned width=0,height=0;};
 std::filesystem::path root;std::map<uint32_t,std::filesystem::path> folders;std::map<std::pair<uint32_t,bool>,Image> cache;
 template<class T>static void release(T*&p){if(p)p->Release();p=nullptr;}
 static Image load(IDirect3DDevice9*d,const std::filesystem::path&file){
  Image out;std::error_code ec;const auto bytes=std::filesystem::file_size(file,ec);if(ec||bytes<8||bytes>64*1024*1024)return out;
  const HRESULT init=CoInitializeEx(nullptr,COINIT_MULTITHREADED);const bool uninit=SUCCEEDED(init);
  IWICImagingFactory* factory=nullptr;IWICBitmapDecoder* decoder=nullptr;IWICBitmapFrameDecode* frame=nullptr;IWICBitmapScaler* scaler=nullptr;IWICFormatConverter* convert=nullptr;
  if(SUCCEEDED(CoCreateInstance(CLSID_WICImagingFactory,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&factory)))&&SUCCEEDED(factory->CreateDecoderFromFilename(file.c_str(),nullptr,GENERIC_READ,WICDecodeMetadataCacheOnDemand,&decoder))&&SUCCEEDED(decoder->GetFrame(0,&frame))){
   UINT w=0,h=0;if(SUCCEEDED(frame->GetSize(&w,&h))&&w&&h&&w<=16384&&h<=16384&&uint64_t(w)*h<=100000000){
    const double ratio=std::min(1.,512./double(std::max(w,h)));const UINT ow=std::max(1u,UINT(w*ratio)),oh=std::max(1u,UINT(h*ratio));IWICBitmapSource*source=frame;
    if((ow!=w||oh!=h)&&SUCCEEDED(factory->CreateBitmapScaler(&scaler))&&SUCCEEDED(scaler->Initialize(frame,ow,oh,WICBitmapInterpolationModeFant)))source=scaler;
    if(SUCCEEDED(factory->CreateFormatConverter(&convert))&&SUCCEEDED(convert->Initialize(source,GUID_WICPixelFormat32bppBGRA,WICBitmapDitherTypeNone,nullptr,0,WICBitmapPaletteTypeCustom))){
     UINT cw=0,ch=0;convert->GetSize(&cw,&ch);if(cw&&ch&&cw<=512&&ch<=512){std::vector<BYTE> pixels(size_t(cw)*ch*4);if(SUCCEEDED(convert->CopyPixels(nullptr,cw*4,UINT(pixels.size()),pixels.data()))&&SUCCEEDED(d->CreateTexture(cw,ch,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&out.texture,nullptr))){D3DLOCKED_RECT locked{};if(SUCCEEDED(out.texture->LockRect(0,&locked,nullptr,0))){for(UINT y=0;y<ch;++y)std::memcpy(static_cast<BYTE*>(locked.pBits)+size_t(y)*locked.Pitch,pixels.data()+size_t(y)*cw*4,size_t(cw)*4);out.texture->UnlockRect(0);out.width=cw;out.height=ch;}else release(out.texture);}}
    }
   }
  }
  release(convert);release(scaler);release(frame);release(decoder);release(factory);if(uninit)CoUninitialize();return out;
 }
public:
 ~VehicleLogos(){reset();}
 void reset(){for(auto&v:cache)release(v.second.texture);cache.clear();}
 void setRoot(const std::filesystem::path&p){reset();root=p;folders.clear();std::error_code ec;if(!std::filesystem::is_directory(root,ec))return;for(std::filesystem::directory_iterator it(root,std::filesystem::directory_options::skip_permission_denied,ec),end;it!=end&&folders.size()<4096;it.increment(ec)){if(ec){ec.clear();continue;}if(it->is_directory(ec)&&!it->is_symlink(ec)){const auto name=it->path().filename().u8string();if(!name.empty()&&name[0]!='.')folders.emplace(modelNameKey(name),it->path());}}}
 IDirect3DTexture9* get(IDirect3DDevice9*d,uint32_t model,bool brand,unsigned&w,unsigned&h){const auto folder=folders.find(model);if(folder==folders.end())return nullptr;const auto key=std::make_pair(model,brand);auto found=cache.find(key);if(found==cache.end()){if(cache.size()>=32){release(cache.begin()->second.texture);cache.erase(cache.begin());}found=cache.emplace(key,load(d,folder->second/(brand?L"brand.png":L"car.png"))).first;}w=found->second.width;h=found->second.height;return found->second.texture;}
};
}
