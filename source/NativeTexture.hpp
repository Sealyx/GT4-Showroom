#pragma once
#include "RuntimeModels.hpp"
#include <d3d9.h>
#include <array>
namespace gt4 {
struct NativeTextureView {
 IDirect3DTexture9* texture=nullptr;float uScale=1,vScale=1,uOffset=0,vOffset=0;
 bool declared=false,unsupported=false;
};
// RwRaster parent/size/offset and the D3D9 raster extension are verified against
// the classic RW SDK. A subraster need not own a second D3D texture.
inline NativeTextureView nativeTexture(void* rwTexture,int extension){
 NativeTextureView out;out.declared=rwTexture!=nullptr;if(!rwTexture)return out;
 void* raster=nullptr;if(extension<4||extension>=4096||!gta::read(uintptr_t(rwTexture),raster)||!raster)return out;
 struct Raster{void*parent;void*pixels;void*palette;int width,height,depth,stride;int16_t x,y;};
 static_assert(sizeof(Raster)==32,"classic Win32 raster prefix");
 Raster first{};if(!gta::read(uintptr_t(raster),first)||first.width<1||first.height<1)return out;
 std::array<void*,16>seen{};size_t n=0;int x=0,y=0;Raster current=first;
 while(current.parent&&current.parent!=raster){
  if(n==seen.size()){out.unsupported=true;return out;}for(size_t i=0;i<n;++i)if(seen[i]==raster){out.unsupported=true;return out;}
  seen[n++]=raster;x+=current.x;y+=current.y;raster=current.parent;if(!gta::read(uintptr_t(raster),current))return out;
 }
 if(current.width<1||current.height<1||x<0||y<0||x+first.width>current.width||y+first.height>current.height){out.unsupported=true;return out;}
 uint8_t cube=0;if(!gta::read(uintptr_t(raster)+size_t(extension)+9,cube))return out;
 if(cube&15){out.unsupported=true;return out;} // Cube resources need a shader adapter.
 if(!gta::read(uintptr_t(raster)+size_t(extension),out.texture))return out;
 out.uScale=float(first.width)/float(current.width);out.vScale=float(first.height)/float(current.height);
 out.uOffset=float(x)/float(current.width);out.vOffset=float(y)/float(current.height);return out;
}
inline bool preparePluginPreview(bool vehfuncs,bool ivf,int kind){return (vehfuncs||ivf)&&(kind==0||kind==1||kind==2||kind==9||kind==10||kind==11);}
}
